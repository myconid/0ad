/* Copyright (C) 2012 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "MessageHandler.h"
#include "../GameLoop.h"
#include "../CommandProc.h"

#include "graphics/GameView.h"
#include "graphics/LOSTexture.h"
#include "graphics/MapWriter.h"
#include "graphics/Patch.h"
#include "graphics/Terrain.h"
#include "graphics/TerrainTextureEntry.h"
#include "graphics/TerrainTextureManager.h"
#include "lib/tex/tex.h"
#include "lib/file/file.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "ps/Loader.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpPlayer.h"
#include "simulation2/components/ICmpPlayerManager.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/components/ICmpTerrain.h"

namespace
{
	void InitGame()
	{
		if (g_Game)
		{
			delete g_Game;
			g_Game = NULL;
		}

		g_Game = new CGame();

		// Default to player 1 for playtesting
		g_Game->SetPlayerID(1);
	}

	void StartGame(const CScriptValRooted& attrs)
	{
		g_Game->StartGame(attrs, "");

		// TODO: Non progressive load can fail - need a decent way to handle this
		LDR_NonprogressiveLoad();
		
		PSRETURN ret = g_Game->ReallyStartGame();
		ENSURE(ret == PSRETURN_OK);

		// Disable fog-of-war
		CmpPtr<ICmpRangeManager> cmpRangeManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
		if (cmpRangeManager)
			cmpRangeManager->SetLosRevealAll(-1, true);
	}
}

namespace AtlasMessage {

QUERYHANDLER(GenerateMap)
{
	InitGame();

	// Random map
	ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();
	
	CScriptValRooted settings = scriptInterface.ParseJSON(*msg->settings);

	CScriptValRooted attrs;
	scriptInterface.Eval("({})", attrs);
	scriptInterface.SetProperty(attrs.get(), "mapType", std::string("random"));
	scriptInterface.SetProperty(attrs.get(), "script", std::wstring(*msg->filename));
	scriptInterface.SetProperty(attrs.get(), "settings", settings, false);

	try
	{
		StartGame(attrs);

		msg->status = 0;
	}
	catch (PSERROR_Game_World_MapLoadFailed e)
	{
		// Cancel loading
		LDR_Cancel();

		// Since map generation failed and we don't know why, use the blank map as a fallback

		InitGame();

		ScriptInterface& si = g_Game->GetSimulation2()->GetScriptInterface();
		CScriptValRooted atts;
		si.Eval("({})", atts);
		si.SetProperty(atts.get(), "mapType", std::string("scenario"));
		si.SetProperty(atts.get(), "map", std::wstring(L"_default"));
		StartGame(atts);

		msg->status = -1;
	}
}

MESSAGEHANDLER(LoadMap)
{
	InitGame();

	// Scenario
	CStrW map = *msg->filename;
	CStrW mapBase = map.BeforeLast(L".pmp"); // strip the file extension, if any

	ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();
	
	CScriptValRooted attrs;
	scriptInterface.Eval("({})", attrs);
	scriptInterface.SetProperty(attrs.get(), "mapType", std::string("scenario"));
	scriptInterface.SetProperty(attrs.get(), "map", std::wstring(mapBase));

	StartGame(attrs);
}

MESSAGEHANDLER(ImportHeightmap)
{
	/*InitGame();

	// Scenario
	CStrW map = *msg->filename;
	CStrW mapBase = map.BeforeLast(L".pmp"); // strip the file extension, if any

	ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();
	
	CScriptValRooted attrs;
	scriptInterface.Eval("({})", attrs);
	scriptInterface.SetProperty(attrs.get(), "mapType", std::string("scenario"));
	scriptInterface.SetProperty(attrs.get(), "map", std::wstring(mapBase));

	StartGame(attrs);*/
	CStrW src = *msg->filename;
	
	std::wcout << src << std::endl;
	
	size_t fileSize;
	shared_ptr<u8> fileData;
	
	File file;
	
	if (file.Open(src, O_RDONLY) < 0)
	{
		LOGERROR(L"Failed to load heightmap.");
		return;
	}
	
	fileSize = lseek(file.Descriptor(), 0, SEEK_END);
	lseek(file.Descriptor(), 0, SEEK_SET);
	
	fileData = shared_ptr<u8>(new u8[fileSize]);
	
	read(file.Descriptor(), fileData.get(), fileSize);

	Tex tex;
	if (tex_decode(fileData, fileSize, &tex) < 0)
	{
		LOGERROR(L"Failed to decode heightmap.");
		return;
	}

	// Check whether there's any alpha channel
	bool hasAlpha = ((tex.flags & TEX_ALPHA) != 0);

	// Convert to uncompressed BGRA with no mipmaps
	if (tex_transform_to(&tex, (tex.flags | TEX_BGR | TEX_ALPHA) & ~(TEX_DXT | TEX_MIPMAPS)) < 0)
	{
		//LOGERROR(L"Failed to transform texture \"%ls\"", src.c_str());
		LOGERROR(L"Failed to transform heightmap.");
		tex_free(&tex);
		return;
	}

	
	u16* heightmap = g_Game->GetWorld()->GetTerrain()->GetHeightMap();
	ssize_t hmSize = g_Game->GetWorld()->GetTerrain()->GetVerticesPerSide();
	
	ssize_t edgeH = std::min(hmSize, (ssize_t)tex.h);
	ssize_t edgeW = std::min(hmSize, (ssize_t)tex.w);
	
	
	u8* mapdata = tex_get_data(&tex);
	ssize_t bytesPP = tex.bpp / 8;
	ssize_t mapLineSkip = tex.w * bytesPP;
	
	for (ssize_t y = 0; y < edgeH; ++y)
	{
		for (ssize_t x = 0; x < edgeW; ++x)
		{
			heightmap[y * hmSize + x] = mapdata[y * mapLineSkip + x * bytesPP] * 256;
		}
	}
	
	
	
	// Check if the texture has all alpha=255, so we can automatically
	// switch from DXT3/DXT5 to DXT1 with no loss
	/*if (hasAlpha)
	{
		hasAlpha = false;
		u8* data = tex_get_data(&tex);
		for (size_t i = 0; i < tex.w * tex.h; ++i)
		{
			if (data[i*4+3] != 0xFF)
			{
				hasAlpha = true;
				break;
			}
		}
	}*/
	
	
	tex_free(&tex);
	
}


MESSAGEHANDLER(SaveMap)
{
	CMapWriter writer;
	const VfsPath pathname = VfsPath("maps/scenarios") / *msg->filename;
	writer.SaveMap(pathname,
		g_Game->GetWorld()->GetTerrain(),
		g_Renderer.GetWaterManager(), g_Renderer.GetSkyManager(),
		&g_LightEnv, g_Game->GetView()->GetCamera(), g_Game->GetView()->GetCinema(),
		g_Game->GetSimulation2());
}

QUERYHANDLER(GetMapSettings)
{
	msg->settings = g_Game->GetSimulation2()->GetMapSettingsString();
}

BEGIN_COMMAND(SetMapSettings)
{
	std::string m_OldSettings, m_NewSettings;
	
	void SetSettings(const std::string& settings)
	{
		g_Game->GetSimulation2()->SetMapSettings(settings);
	}

	void Do()
	{
		m_OldSettings = g_Game->GetSimulation2()->GetMapSettingsString();
		m_NewSettings = *msg->settings;
		
		SetSettings(m_NewSettings);
	}

	// TODO: we need some way to notify the Atlas UI when the settings are changed
	//	externally, otherwise this will have no visible effect
	void Undo()
	{
		// SetSettings(m_OldSettings);
	}

	void Redo()
	{
		// SetSettings(m_NewSettings);
	}

	void MergeIntoPrevious(cSetMapSettings* prev)
	{
		prev->m_NewSettings = m_NewSettings;
	}
};
END_COMMAND(SetMapSettings)

MESSAGEHANDLER(LoadPlayerSettings)
{
	g_Game->GetSimulation2()->LoadPlayerSettings(msg->newplayers);
}

QUERYHANDLER(GetMapSizes)
{
	msg->sizes = g_Game->GetSimulation2()->GetMapSizes();
}

QUERYHANDLER(GetRMSData)
{
	msg->data = g_Game->GetSimulation2()->GetRMSData();
}

BEGIN_COMMAND(ResizeMap)
{
	int m_OldTiles, m_NewTiles;

	cResizeMap()
	{
	}

	void MakeDirty()
	{
		CmpPtr<ICmpTerrain> cmpTerrain(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
		if (cmpTerrain)
			cmpTerrain->ReloadTerrain();

		// The LOS texture won't normally get updated when running Atlas
		// (since there's no simulation updates), so explicitly dirty it
		g_Game->GetView()->GetLOSTexture().MakeDirty();
	}

	void ResizeTerrain(int tiles)
	{
		CTerrain* terrain = g_Game->GetWorld()->GetTerrain();

		terrain->Resize(tiles / PATCH_SIZE);

		MakeDirty();
	}

	void Do()
	{
		CmpPtr<ICmpTerrain> cmpTerrain(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
		if (!cmpTerrain)
		{
			m_OldTiles = m_NewTiles = 0;
		}
		else
		{
			m_OldTiles = (int)cmpTerrain->GetTilesPerSide();
			m_NewTiles = msg->tiles;
		}

		ResizeTerrain(m_NewTiles);
	}

	void Undo()
	{
		ResizeTerrain(m_OldTiles);
	}

	void Redo()
	{
		ResizeTerrain(m_NewTiles);
	}
};
END_COMMAND(ResizeMap)

QUERYHANDLER(VFSFileExists)
{
	msg->exists = VfsFileExists(*msg->path);
}

}
