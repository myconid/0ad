// Public interface to almost all of AtlasObject.
// (See AtlasObjectText for the rest of it).
//
// Tries to include as few headers as possible, to minimise its impact
// on compile times.

#ifndef INCLUDED_ATLASOBJECT
#define INCLUDED_ATLASOBJECT

#include <wchar.h> // for wchar_t
#include <string>

//////////////////////////////////////////////////////////////////////////
// Mostly-private bits:

// Helper class to let us define a conversion operator only for AtSmartPtr<not const>
template<class ConstSmPtr, class T> struct ConstCastHelper { operator ConstSmPtr (); };
template<class ConstSmPtr, class T> struct ConstCastHelper<ConstSmPtr, const T> { };

// Simple reference-counted pointer. class T must contain a reference count,
// initialised to 0. An external implementation (in AtlasObjectImpl.cpp)
// provides the inc_ref and dec_ref methods, so that this header file doesn't
// need to know their implementations.
template<class T> class AtSmartPtr : public ConstCastHelper<AtSmartPtr<const T>, T>
{
	friend struct ConstCastHelper<AtSmartPtr<const T>, T>;
public:
	// Constructors
	AtSmartPtr() : ptr(NULL) {}
	explicit AtSmartPtr(T* p) : ptr(p) { inc_ref(); }
	// Copy constructor
	AtSmartPtr(const AtSmartPtr<T>& r) : ptr(r.ptr) { inc_ref(); }
	// Assignment operators
	AtSmartPtr<T>& operator=(T* p) { dec_ref(); ptr = p; inc_ref(); return *this; }
	AtSmartPtr<T>& operator=(const AtSmartPtr<T>& r) { if (&r != this) { dec_ref(); ptr = r.ptr; inc_ref(); } return *this; }
	// Destructor
	~AtSmartPtr() { dec_ref(); }
	// Allow conversion from non-const T* to const T*
	//operator AtSmartPtr<const T> () { return AtSmartPtr<const T>(ptr); } // (actually provided by ConstCastHelper)
	// Override ->
	T* operator->() const { return ptr; }
	// Test whether the pointer is pointing to anything
	operator bool() const { return ptr!=NULL; }
	bool operator!() const { return ptr==NULL; }
private:
	void inc_ref();
	void dec_ref();
	T* ptr;
};

template<class ConstSmPtr, class T>
ConstCastHelper<ConstSmPtr, T>::operator ConstSmPtr ()
{
	return ConstSmPtr(static_cast<AtSmartPtr<T>*>(this)->ptr);
}

// A few required declarations
class AtObj;
class AtNode;
class AtIterImpl;


//////////////////////////////////////////////////////////////////////////
// Public bits:


// AtIter is an iterator over AtObjs - use it like:
//
//     for (AtIter thing = whatever["thing"]; thing.defined(); ++thing)
//         DoStuff(thing);
//
// to handle XML data like:
//
//   <whatever>
//     <thing>Stuff 1</thing>
//     <thing>Stuff 2</thing>
//   </whatever>

class AtIter
{
public:
	// Increment the iterator; or make it undefined, if there weren't any
	// AtObjs left to iterate over
	AtIter& operator++ ();
	// Return whether this iterator has an AtObj to point to
	bool defined() const;
	// Return whether this iterator is pointing to a non-contentless AtObj
	bool hasContent() const;

	// Return an iterator to the children matching 'key'. (That is, children
	// of the AtObj currently pointed to by this iterator)
	const AtIter operator[] (const char* key) const;

	// Return the AtObj currently pointed to by this iterator
	const AtObj operator* () const;

	// Return the string value of the AtObj currently pointed to by this iterator
	operator const wchar_t* () const;

	// Private implementation. (But not 'private:', because it's a waste of time
	// adding loads of friend functions)
	AtSmartPtr<AtIterImpl> p;
};


class AtObj
{
public:
	AtObj() {}
	AtObj(const AtObj& r) : p(r.p) {}

	// Return an iterator to the children matching 'key'
	const AtIter operator[] (const char* key) const;

	// Return the string value of this object
	operator const wchar_t* () const;

	// Check whether the object contains anything (even if those things are empty)
	bool defined() const { return (bool)p; }
	
	// Check recursively whether there's actually any non-empty data in the object
	bool hasContent() const;

	// Add or set a child. The wchar_t* versions create a new AtObj with
	// the appropriate string value, then use that as the child.
	//
	// These alter the AtObj's internal pointer, and the pointed-to data is
	// never actually altered. Copies of this AtObj (including copies stored
	// inside other AtObjs) will not be affected.
	void add(const char* key, const wchar_t* value);
	void add(const char* key, AtObj& data);
	void set(const char* key, const wchar_t* value);
	void set(const char* key, AtObj& data);
	void setString(const wchar_t* value);

	AtSmartPtr<const AtNode> p;
};


// Miscellaneous utility functions:
namespace AtlasObject
{
	// Returns AtObj() on failure - test with AtObj::defined()
	AtObj LoadFromXML(const std::string& xml);

	// Returns UTF-8-encoded XML document string.
	// Returns empty string on failure.
	std::string SaveToXML(AtObj& obj);

	AtObj TrimEmptyChildren(AtObj& obj);
}

#endif // INCLUDED_ATLASOBJECT
