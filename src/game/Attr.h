//
//  Attr.h
//  OpenLieroX
//
//  Created by Albert Zeyer on 17.01.12.
//  code under LGPL
//

#ifndef OpenLieroX_Attr_h
#define OpenLieroX_Attr_h

#include <stdint.h>
#include <string>
#include <boost/function.hpp>
#include "util/WeakRef.h"
#include "CScriptableVars.h"
#include "gusanos/luaapi/classes.h"
#include "util/macros.h"
#include "util/BaseObject.h"

struct AttrExt {
	bool updated;
	AttrExt() : updated(false) {}
};

struct AttrDesc {
	typedef uint32_t ObjTypeId;
	typedef uint32_t AttrId;

	ObjTypeId objTypeId;
	ScriptVarType_t attrType;
	intptr_t attrMemOffset;
	intptr_t attrExtMemOffset;
	std::string attrName;
	AttrId attrId;
	ScriptVar_t defaultValue;

	bool serverside;
	boost::function<bool(void* base, const AttrDesc* attrDesc, ScriptVar_t oldValue)> onUpdate;
	boost::function<void(void* base, const AttrDesc* attrDesc)> sync;
	
	AttrDesc()
	: objTypeId(0), attrType(SVT_INVALID), attrMemOffset(0), attrId(0) {}

	void* getValuePtr(void* base) const { return (void*)(uintptr_t(base) + attrMemOffset); }
	AttrExt* getAttrExtPtr(void* base) const { return (AttrExt*)(uintptr_t(base) + attrMemOffset + attrExtMemOffset); }
	ScriptVarPtr_t get(void* base) const { return ScriptVarPtr_t(attrType, (void*)getValuePtr(base), defaultValue); }

	std::string description() const;
};

struct AttribRef {
	AttrDesc::ObjTypeId objTypeId;
	AttrDesc::AttrId attrId;
};

void registerAttrDesc(AttrDesc& attrDesc);
const AttrDesc* getAttrDesc(const AttribRef& attrRef);
void pushObjAttrUpdate(WeakRef<BaseObject> obj);
void iterAttrUpdates(boost::function<void(BaseObject*, const AttrDesc* attrDesc, ScriptVar_t oldValue)> callback);

template <typename T, typename AttrDescT>
struct Attr {
	typedef T ValueType;
	typedef AttrDescT AttrDescType;
	T value;
	AttrExt ext;
	Attr() {
		// also inits attrDesc which is needed here
		value = (T) attrDesc()->defaultValue;
	}
	const AttrDescT* attrDesc() {
		static const AttrDescT desc;
		return &desc;
	}
	BaseObject* parent() { return (BaseObject*)(uintptr_t(this) - attrDesc()->attrMemOffset); }
	T get() const { return value; }
	operator T() const { return get(); }
	T& write() {
		if(parent()->attrUpdates.empty())
			pushObjAttrUpdate(parent()->thisWeakRef);
		if(!ext.updated || parent()->attrUpdates.empty()) {
			assert(&value == attrDesc()->getValuePtr(parent()));
			assert(&ext == attrDesc()->getAttrExtPtr(parent()));
			AttrUpdateInfo info;
			info.attrDesc = attrDesc();
			info.oldValue = ScriptVar_t(value);
			parent()->attrUpdates.push_back(info);
			ext.updated = true;
		}
		return value;
	}
	Attr& operator=(const T& v) {
		write() = v;
		return *this;
	}
};

template <typename T, typename AttrDescT>
struct AttrWithBasicOpts : public Attr<T, AttrDescT> {
	void operator++(int) { *this += 1; }
	void operator--(int) { *this -= 1; }
	template<typename T2> AttrWithBasicOpts& operator+=(T2 i) { *this = this->value + i; return *this; }
	template<typename T2> AttrWithBasicOpts& operator-=(T2 i) { *this = this->value - i; return *this; }
	template<typename T2> AttrWithBasicOpts& operator*=(T2 i) { *this = this->value * i; return *this; }
	template<typename T2> AttrWithBasicOpts& operator/=(T2 i) { *this = this->value / i; return *this; }
	template<typename T2> AttrWithBasicOpts& operator=(const T2& v) { Attr<T, AttrDescT>::operator=(v); return *this; }
	template<typename T2> T operator+(T2 o) const { return this->value + o; }
	template<typename T2> T operator-(T2 o) const { return this->value - o; }
	template<typename T2> T operator*(T2 o) const { return this->value * o; }
	template<typename T2> T operator/(T2 o) const { return this->value / o; }
	bool operator==(T o) const { return this->value == o; }
	bool operator!=(T o) const { return this->value != o; }
};

template <typename T, typename AttrDescT>
struct AttrWithIntOpts : public Attr<T, AttrDescT> {
	void operator++(int) { *this += 1; }
	void operator--(int) { *this -= 1; }
	template<typename T2> AttrWithIntOpts& operator+=(T2 i) { *this = this->value + i; return *this; }
	template<typename T2> AttrWithIntOpts& operator-=(T2 i) { *this = this->value - i; return *this; }
	template<typename T2> AttrWithIntOpts& operator*=(T2 i) { *this = this->value * i; return *this; }
	template<typename T2> AttrWithIntOpts& operator/=(T2 i) { *this = this->value / i; return *this; }
	template<typename T2> AttrWithIntOpts& operator%=(T2 i) { *this = this->value % i; return *this; }
	template<typename T2> AttrWithIntOpts& operator=(const T2& v) { Attr<T, AttrDescT>::operator=(v); return *this; }
	template<typename T2> T operator+(T2 o) const { return this->value + o; }
	template<typename T2> T operator-(T2 o) const { return this->value - o; }
	template<typename T2> T operator*(T2 o) const { return this->value * o; }
	template<typename T2> T operator/(T2 o) const { return this->value / o; }
	bool operator==(T o) const { return this->value == o; }
	bool operator!=(T o) const { return this->value != o; }
};

template <typename T, typename AttrDescT>
struct AttrWithMaybeOpts {
	typedef Attr<T, AttrDescT> Type;
};

#define USE_OPTS_FOR(T, k) \
template <typename AttrDescT> \
struct AttrWithMaybeOpts<T,AttrDescT> { \
	typedef AttrWith ## k ## Opts<T,AttrDescT> Type; \
}

USE_OPTS_FOR(int, Int);
USE_OPTS_FOR(float, Basic);
USE_OPTS_FOR(CVec, Basic);

#undef USE_OPTS_FOR


#define ATTR(parentType, type, name, id, attrDescSpecCode) \
struct _ ## parentType ## _AttrDesc ## id : AttrDesc { \
	_ ## parentType ## _AttrDesc ## id () { \
		objTypeId = LuaID<parentType>::value; \
		attrType = GetType<type>::value; \
		attrMemOffset = (intptr_t)__OLX_OFFSETOF(parentType, name); \
		struct Dummy { type x; AttrExt ext; }; \
		attrExtMemOffset = (intptr_t)__OLX_OFFSETOF(Dummy, ext); \
		attrName = #name ; \
		attrId = id; \
		defaultValue = ScriptVar_t(type()); \
		attrDescSpecCode; \
		registerAttrDesc(*this); \
	} \
}; \
typedef AttrWithMaybeOpts<type, _ ## parentType ## _AttrDesc ## id>::Type name ## _Type; \
name ## _Type name;

#endif
