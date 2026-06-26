/*
 * LuaProcessor.cpp
 *
 *  Created on: Jun 29, 2011
 *      Author: QVXLabs LLC
 *   Copyright: (c)2012 QVXLabs LLC
 *
 *  This file is part of xsd-tools.
 *
 *  xsd-tools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  xsd-tools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with xsd-tools.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sstream>
#include <memory>
#include <assert.h>
#include "./src/XSDParser/Parser.hpp"
#include "./src/XSDParser/Elements/Node.hpp"
#include "./src/XSDParser/Elements/Schema.hpp"
#include "./src/XSDParser/Elements/Element.hpp"
#include "./src/XSDParser/Elements/Sequence.hpp"
#include "./src/XSDParser/Elements/Union.hpp"
#include "./src/XSDParser/Elements/Restriction.hpp"
#include "./src/XSDParser/Elements/Choice.hpp"
#include "./src/XSDParser/Elements/Attribute.hpp"
#include "./src/XSDParser/Elements/List.hpp"
#include "./src/XSDParser/Elements/SimpleType.hpp"
#include "./src/XSDParser/Elements/ComplexType.hpp"
#include "./src/XSDParser/Elements/Group.hpp"
#include "./src/XSDParser/Elements/Any.hpp"
#include "./src/XSDParser/Elements/ComplexContent.hpp"
#include "./src/XSDParser/Elements/Extension.hpp"
#include "./src/XSDParser/Elements/SimpleContent.hpp"
#include "./src/XSDParser/Elements/MinExclusive.hpp"
#include "./src/XSDParser/Elements/MaxExclusive.hpp"
#include "./src/XSDParser/Elements/MinInclusive.hpp"
#include "./src/XSDParser/Elements/MaxInclusive.hpp"
#include "./src/XSDParser/Elements/MinLength.hpp"
#include "./src/XSDParser/Elements/MaxLength.hpp"
#include "./src/XSDParser/Elements/Length.hpp"
#include "./src/XSDParser/Elements/Enumeration.hpp"
#include "./src/XSDParser/Elements/FractionDigits.hpp"
#include "./src/XSDParser/Elements/Pattern.hpp"
#include "./src/XSDParser/Elements/TotalDigits.hpp"
#include "./src/XSDParser/Elements/WhiteSpace.hpp"
#include "./src/XSDParser/Elements/AttributeGroup.hpp"
#include "./src/XSDParser/Elements/Include.hpp"
#include "./src/XSDParser/Elements/Annotation.hpp"
#include "./src/XSDParser/Elements/Documentation.hpp"
#include "./src/XSDParser/Elements/All.hpp"
#include "./src/Processors/LuaProcessor.hpp"
#include "./src/Processors/LuaAdapter.hpp"
#include "./src/Processors/SimpleTypeExtracter.hpp"
#include "./src/Processors/ArrayType.hpp"

using namespace std;
using namespace Processors;

/* First <annotation><documentation> text directly under pNode, or "" if none.
   Annotations aren't part of the Lua model the visitor builds, so read them on
   demand where the owning construct (element/complexType/attribute) is
   processed (#4). */
static string documentationOf_(const XSD::Elements::Node* pNode) {
	string doc;
	pNode->eachChild_([&doc](const XSD::Elements::Node& rChild) {
		if (!(XSD_ISELEMENT(&rChild, XSD::Elements::Annotation)))
			return;
		rChild.eachChild_([&doc](const XSD::Elements::Node& rGrand) {
			if (doc.empty() &&
			    XSD_ISELEMENT(&rGrand, XSD::Elements::Documentation)) {
				doc = static_cast<const XSD::Elements::Documentation&>(rGrand)
				          .DocumentationStr();
			}
		});
	});
	return doc;
}

/* The backing XML element of a user-defined complex/simple type, or NULL for
   builtins/arrays (used as a cycle key for type-derivation chains). */
static const void* typeKey_(const XSD::Types::BaseType& rType) {
	if (XSD_ISTYPE(&rType, XSD::Types::ComplexType))
		return &dynamic_cast<const XSD::Types::ComplexType&>(rType)
		            .pValue_->GetXMLElm();
	if (XSD_ISTYPE(&rType, XSD::Types::SimpleType))
		return &dynamic_cast<const XSD::Types::SimpleType&>(rType)
		            .pValue_->GetXMLElm();
	return NULL;
}

/* RAII insert/erase of a key on the shared active-path set (exception-safe
   marking for the recursion/derivation guards). */
namespace {
	struct PathMark {
		std::set<const void*>&	path_;
		const void*				key_;
		PathMark(std::set<const void*>& rPath, const void* pKey)
			: path_(rPath), key_(pKey) { if (key_) path_.insert(key_); }
		~PathMark() { if (key_) path_.erase(key_); }
	};
}

LuaProcessor::LuaProcessor(lua_State * pLuaState)
	: LuaProcessorBase(new LuaAdapter(pLuaState))
	, activePath_(new set<const void*>())
{ }

LuaProcessor::LuaProcessor(const LuaProcessor& rProcessor)
	: LuaProcessorBase(rProcessor)
	, activePath_(rProcessor.activePath_)
{ }

LuaProcessor::LuaProcessor(LuaAdapter * pLuaAdapter)
	: LuaProcessorBase(pLuaAdapter)
	, activePath_(new set<const void*>())
{ }

/* virtual */
LuaProcessor::~LuaProcessor() {
}

/* virtual */ void
LuaProcessor::ProcessSchema(const XSD::Elements::Schema* pNode) {
	if (pNode->isRootSchema()) {
		/* create root schema table and append it to stack */
		LuaProcessor processor(luaAdapter_()->Schema());
		processor.activePath_ = activePath_;
		pNode->ParseChildren(processor);
	} else {
		/* parse schema children */
		pNode->ParseChildren(*this);
	}
}

/* virtual */ void
LuaProcessor::ProcessElement(const XSD::Elements::Element* pNode) {
	/* don't process element if it can't occur */
	if (pNode->HasMaxOccurs() && (0 == pNode->MaxOccurs()))
		return;
	/* output the element */
	if (!pNode->HasRef()) {
		unique_ptr<XSD::Types::BaseType> pElmType(pNode->Type());
		/* process element type */
		/* inserts basic type. Handle array types the same as basic types */
		LuaContent * pLuaContent = dynamic_cast<LuaContent*>(luaAdapter_());
		if (NULL == pLuaContent) {
			/* enter the type content table & add elements, then leave */
			LuaType * pLuaType = dynamic_cast<LuaType*>(luaAdapter_());
			LuaProcessor processor(pLuaType->Content());
			processor.activePath_ = activePath_;
			processor.ProcessElement(pNode);
		} else {
			/* Default min/maxOccurs is 1 */
			int maxOccurs = pNode->HasMaxOccurs() ?	pNode->MaxOccurs() : 1;
			int minOccurs = pNode->HasMinOccurs() ?	pNode->MinOccurs() : 1;
			LuaType * pLuaType =
				pLuaContent->Type(pNode->Name(), maxOccurs, minOccurs);
			LuaProcessor luaPrcssr(pLuaType);
			luaPrcssr.activePath_ = activePath_;
			/* element namespace/form/doc land on the element's type table
			   before parseType_ descends into the content sub-tables. Setting
			   the element's doc first lets it win over the referenced type's
			   own annotation (LuaType::Documentation won't overwrite). */
			pLuaType->Namespace(pNode->Namespace());
			pLuaType->Qualified(pNode->Qualified());
			pLuaType->Documentation(documentationOf_(pNode));
			/* Recursion: if this very element is already being expanded higher
			   on the path, emit a by-name reference instead of expanding it
			   again (which would not terminate). Otherwise mark it and expand;
			   trnsfrm_old resolves the reference against the flat registry. */
			const void* pElmKey = &pNode->GetXMLElm();
			if (0 != activePath_->count(pElmKey)) {
				/* reference the enclosing element's type by name; trnsfrm_old
				   links it to that element's flat-registry entry. */
				pLuaType->TypeRef(pNode->Name());
			} else {
				PathMark mark(*activePath_, pElmKey);
				luaPrcssr.parseType_(*pElmType);
			}
		}
	} else {
		unique_ptr<XSD::Elements::Element> pRefElm(pNode->RefElement());
		ProcessElement(pRefElm.get());
	}
}

/* virtual */ void
LuaProcessor::ProcessUnion(const XSD::Elements::Union* pNode) {
	parseType_(XSD::Types::String());
}

/* virtual */ void
LuaProcessor::ProcessRestriction(const XSD::Elements::Restriction* pNode ) {

	if (pNode->isParentComplexContent()) {
		pNode->ParseChildren(*this);
	} else if (pNode->isParentSimpleContent()) {
		pNode->ParseChildren(*this);
		unique_ptr<XSD::Types::BaseType> pType(pNode->Base());
		parseType_(*pType);
	} else {
		/* Walk facet children directly (accumulating onto facets_ across
		   the derivation chain) before resolving the base type. Bypasses
		   Restriction::ParseChildren, whose numeric-base verification
		   rejects valid facets on integer-derived types. parseType_
		   recurses through nested restrictions on this same processor, so
		   facets from every level land on the leaf type. */
		walkFacets_(pNode);
		unique_ptr<XSD::Types::BaseType> pType(pNode->Base());
		parseType_(*pType);
	}
}

/* Dispatch each facet child of a simpleType restriction to this processor,
   so the ProcessXxx facet overrides record their values. */
void
LuaProcessor::walkFacets_(const XSD::Elements::Node* pNode) {
	pNode->eachChild_([this](const XSD::Elements::Node& rChild) {
		if (XSD_ISELEMENT(&rChild, XSD::Elements::MinExclusive) ||
			XSD_ISELEMENT(&rChild, XSD::Elements::MaxExclusive) ||
			XSD_ISELEMENT(&rChild, XSD::Elements::MinInclusive) ||
			XSD_ISELEMENT(&rChild, XSD::Elements::MaxInclusive) ||
			XSD_ISELEMENT(&rChild, XSD::Elements::MinLength) ||
			XSD_ISELEMENT(&rChild, XSD::Elements::MaxLength) ||
			XSD_ISELEMENT(&rChild, XSD::Elements::Length) ||
			XSD_ISELEMENT(&rChild, XSD::Elements::Enumeration) ||
			XSD_ISELEMENT(&rChild, XSD::Elements::Pattern) ||
			XSD_ISELEMENT(&rChild, XSD::Elements::TotalDigits) ||
			XSD_ISELEMENT(&rChild, XSD::Elements::FractionDigits) ||
			XSD_ISELEMENT(&rChild, XSD::Elements::WhiteSpace)) {
			rChild.ParseElement(*this);
		}
	});
}

/* virtual */ void
LuaProcessor::ProcessList(const XSD::Elements::List* pNode) {
	unique_ptr<XSD::Types::BaseType> pTypeLst(pNode->ItemType());
	/* extract xsd native type from simple type */
	SimpleTypeExtracter typeXtr;
	/* create array type */
	parseType_(Types::ArrayType(typeXtr.Extract(*pTypeLst)));
}

/* virtual */ void 
LuaProcessor::ProcessSequence(const XSD::Elements::Sequence * pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void 
LuaProcessor::ProcessChoice(const XSD::Elements::Choice * pNode) {
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessor::ProcessAttribute(const XSD::Elements::Attribute* pNode) {
	if (pNode->HasRef()) {
		unique_ptr<XSD::Elements::Attribute> pRefAtt(pNode->RefAttribute());
		ProcessAttribute(pRefAtt.get());
	} else {
		/* extract xsd native type from simple type and add it */
		SimpleTypeExtracter typeXtr;
		unique_ptr<XSD::Types::BaseType> pType(pNode->Type());
		LuaType * pLuaType = dynamic_cast<LuaType*>(luaAdapter_());
		/* attributes attach to a type table; in a content-only context (e.g. a
		   mixed complexType, which re-enters with a LuaContent adapter) there is
		   no type to attach to. Skip rather than dereference null — mixed-
		   content attributes aren't modelled (documented limitation). */
		if (NULL == pLuaType)
			return;
		unique_ptr<string> pDefault(nullptr);
		unique_ptr<string> pFixed(nullptr);
		unique_ptr<string> pUse(new string("optional"));
		if (pNode->HasDefault()) {
			pDefault.reset(new string(pNode->Default()));
		}
		if (pNode->HasFixed()) {
			pFixed.reset(new string(pNode->Fixed()));
		}
		if (pNode->HasUse()) {
			switch (pNode->Use()) {
			case XSD::Elements::Attribute::Optional:
				pUse.reset(new string("optional"));
				break;
			case XSD::Elements::Attribute::Prohibited:
				pUse.reset(new string("prohibited"));
				break;
			case XSD::Elements::Attribute::Required:
				pUse.reset(new string("required"));
				break;
			default:
				assert(	(pNode->Use() != XSD::Elements::Attribute::Optional) &&
						(pNode->Use() != XSD::Elements::Attribute::Prohibited) &&
						(pNode->Use() != XSD::Elements::Attribute::Required));
			}
		}
		unique_ptr<LuaAttribute> pAttribute(
			pLuaType->Attribute(pNode->Name(),
								typeXtr.Extract(*pType),
								pDefault.get(),
								pFixed.get(),
								pUse.get()
							)
			);
		/* bridge the attribute's restriction facets (clear first so prior
		   content facets don't leak in), then attach to the attribute */
		facets_.clear();
		walkAttributeFacets_(pType.get());
		pAttribute->Facets(facets_);
		facets_.clear();
		/* attribute namespace/form/doc, gated like facets */
		pAttribute->Namespace(pNode->Namespace());
		pAttribute->Qualified(pNode->Qualified());
		pAttribute->Documentation(documentationOf_(pNode));
	}
}

/* Accumulate the facets of an attribute's simpleType restriction onto
   facets_, following the derivation chain when a restriction's base is
   itself a user-defined simpleType (covers inline and named-type
   attributes uniformly: both resolve to a Types::SimpleType). */
void
LuaProcessor::walkAttributeFacets_(const XSD::Types::BaseType* pType) {
	if (!(XSD_ISTYPE(pType, XSD::Types::SimpleType)))
		return;
	const XSD::Types::SimpleType* pSimpleType =
		dynamic_cast<const XSD::Types::SimpleType*>(pType);
	const XSD::Elements::SimpleType* pElm = pSimpleType->pValue_;
	unique_ptr<XSD::Elements::Restriction> pRestriction(pElm->GetRestriction());
	if (NULL == pRestriction.get())
		return;
	walkFacets_(pRestriction.get());
	/* follow the chain when the base is another user-defined simpleType */
	unique_ptr<XSD::Types::BaseType> pBase(pRestriction->Base());
	if (XSD_ISTYPE(pBase.get(), XSD::Types::SimpleType))
		walkAttributeFacets_(pBase.get());
}

/* virtual */ void 
LuaProcessor::ProcessComplexType(const XSD::Elements::ComplexType* pNode) {
	/* add a string to the content type if it is mixed mode */
	if (pNode->Mixed()) {
		/* inserts basic type. Handle array types the same as basic types */
		LuaContent * pLuaContent = dynamic_cast<LuaContent*>(luaAdapter_());
		if (NULL == pLuaContent) {
			/* enter the type content table & add elements, then leave */
			LuaType * pLuaType = dynamic_cast<LuaType*>(luaAdapter_());
			LuaProcessor processor(pLuaType->Content());
			processor.activePath_ = activePath_;
			processor.ProcessComplexType(pNode);
		} else {
			unique_ptr<XSD::Types::String> pType(new XSD::Types::String());
			delete (pLuaContent->Type(pType->Name(), 1));
			pNode->ParseChildren(*this);
		}
	} else {
		/* a named complexType's own annotation, applied to the element's type
		   table (no-op if the element already supplied a doc) */
		LuaType * pLuaType = dynamic_cast<LuaType*>(luaAdapter_());
		if (NULL != pLuaType)
			pLuaType->Documentation(documentationOf_(pNode));
		pNode->ParseChildren(*this);
	}
}

/* virtual */ void
LuaProcessor::ProcessGroup(const XSD::Elements::Group* pNode) {
	/* only parse groups when they are referred */
	if (pNode->HasRef()) {
		unique_ptr<XSD::Elements::Group> pGroup(pNode->RefGroup());
		pGroup->ParseChildren(*this);
	}
}

/* virtual */ void
LuaProcessor::ProcessAny(const XSD::Elements::Any* pNode) {
	/* an any type can be any element in the schema as a child */
	pNode->ParseChildren(*this);
	/* if any is strict, then only the elements allowed in the schema doc tree are valid */
	if (XSD::Elements::Any::Strict == pNode->ProcessContents()) {
		ElementExtracter::ElementLst elmLst = pNode->GetAllowedElements();
		for (	ElementExtracter::ElementLst::iterator itr = elmLst.begin();
				itr != elmLst.end();
				++itr) {
			ProcessElement(*itr);
			delete *itr;
		}
	}
}

/* virtual */ void
LuaProcessor::ProcessExtension(const XSD::Elements::Extension* pNode) {
	unique_ptr<XSD::Types::BaseType> pBase(pNode->Base());
	/* Derivation cycle guard: a base already being resolved on this path
	   (A extends B extends A) is a genuine cycle the inline-expanding model
	   cannot represent — reject cleanly rather than overflow the stack. */
	const void* pBaseKey = typeKey_(*pBase);
	if (pBaseKey && 0 != activePath_->count(pBaseKey))
		throw XSD::XMLException(pNode->GetXMLElm(),
			XSD::XMLException::CyclicTypeDefinition);
	PathMark mark(*activePath_, pBaseKey);
	parseType_(*pBase);
	pNode->ParseChildren(*this);
}

/* virtual */ void
LuaProcessor::ProcessAttributeGroup(const XSD::Elements::AttributeGroup* pNode) {
	/* only parse attribute groups when they are referred */
	if (pNode->HasRef()) {
		unique_ptr<XSD::Elements::AttributeGroup> pGroup(pNode->RefGroup());
		pGroup->ParseChildren(*this);
	}
}

/* virtual */ void
LuaProcessor::ProcessInclude(const XSD::Elements::Include* pNode) {
	const XSD::Elements::Schema * pSchema = pNode->QuerySchema();
	pSchema->ParseChildren(*this);
}

/* virtual */ void 
LuaProcessor::ProcessAll(const XSD::Elements::All * pNode) {
	pNode->ParseChildren(*this);
}

/* stringify a numeric facet value (ostream drops trailing zeros) */
template <typename T>
static string numStr_(T val) {
	ostringstream ss;
	ss << val;
	return ss.str();
}

/* virtual */ void
LuaProcessor::ProcessMinExclusive(const XSD::Elements::MinExclusive* pNode) {
	if (pNode->HasValue())
		facets_.addScalar("minExclusive", numStr_(pNode->Value()));
}

/* virtual */ void
LuaProcessor::ProcessMaxExclusive(const XSD::Elements::MaxExclusive* pNode) {
	if (pNode->HasValue())
		facets_.addScalar("maxExclusive", numStr_(pNode->Value()));
}

/* virtual */ void
LuaProcessor::ProcessMinInclusive(const XSD::Elements::MinInclusive* pNode) {
	if (pNode->HasValue())
		facets_.addScalar("minInclusive", numStr_(pNode->Value()));
}

/* virtual */ void
LuaProcessor::ProcessMaxInclusive(const XSD::Elements::MaxInclusive* pNode) {
	if (pNode->HasValue())
		facets_.addScalar("maxInclusive", numStr_(pNode->Value()));
}

/* virtual */ void
LuaProcessor::ProcessMinLength(const XSD::Elements::MinLength* pNode) {
	if (pNode->HasValue())
		facets_.addScalar("minLength", numStr_(pNode->Value()));
}

/* virtual */ void
LuaProcessor::ProcessMaxLength(const XSD::Elements::MaxLength* pNode) {
	if (pNode->HasValue())
		facets_.addScalar("maxLength", numStr_(pNode->Value()));
}

/* virtual */ void
LuaProcessor::ProcessLength(const XSD::Elements::Length* pNode) {
	if (pNode->HasValue())
		facets_.addScalar("length", numStr_(pNode->Value()));
}

/* virtual */ void
LuaProcessor::ProcessTotalDigits(const XSD::Elements::TotalDigits* pNode) {
	if (pNode->HasValue())
		facets_.addScalar("totalDigits", numStr_(pNode->Value()));
}

/* virtual */ void
LuaProcessor::ProcessFractionDigits(const XSD::Elements::FractionDigits* pNode) {
	if (pNode->HasValue())
		facets_.addScalar("fractionDigits", numStr_(pNode->Value()));
}

/* virtual */ void
LuaProcessor::ProcessWhiteSpace(const XSD::Elements::WhiteSpace* pNode) {
	if (!pNode->HasValue())
		return;
	const char * pVal = "preserve";
	switch (pNode->Value()) {
	case XSD::Elements::WhiteSpace::PRESERVE: pVal = "preserve"; break;
	case XSD::Elements::WhiteSpace::REPLACE:  pVal = "replace";  break;
	case XSD::Elements::WhiteSpace::COLLAPSE: pVal = "collapse"; break;
	}
	facets_.addScalar("whiteSpace", pVal);
}

/* virtual */ void
LuaProcessor::ProcessEnumeration(const XSD::Elements::Enumeration* pNode) {
	if (pNode->HasValue())
		facets_.addToList("enumeration", pNode->Value());
}

/* virtual */ void
LuaProcessor::ProcessPattern(const XSD::Elements::Pattern* pNode) {
	if (pNode->HasValue())
		facets_.addToList("pattern", pNode->Value());
}

/* virtual */ void
LuaProcessor::parseType_(const XSD::Types::BaseType& rXSDType) {
	/* RAII marker: a type is "on the active path" while its content is
	   expanded. The model inline-expands every referenced type, so a cyclic
	   schema (mutual or self-referential) would otherwise recurse until the
	   stack overflows. Keyed on the backing TiXmlElement, because each
	   resolution of a named type yields a fresh Node wrapper. */
	struct ActiveMark {
		std::set<const void*>&	path_;
		const void*				key_;
		ActiveMark(std::set<const void*>& rPath, const void* pKey)
			: path_(rPath), key_(pKey) { path_.insert(pKey); }
		~ActiveMark() { path_.erase(key_); }
	};
	if(XSD_ISTYPE(&rXSDType, XSD::Types::SimpleType)) {
		const XSD::Types::SimpleType* pSimpleType =
			dynamic_cast<const XSD::Types::SimpleType*>(&rXSDType);
		const void* pKey = &pSimpleType->pValue_->GetXMLElm();
		if (0 != activePath_->count(pKey))
			throw XSD::XMLException(pSimpleType->pValue_->GetXMLElm(),
				XSD::XMLException::CyclicTypeDefinition);
		ActiveMark mark(*activePath_, pKey);
		pSimpleType->pValue_->ParseElement(*this);
	} else if(XSD_ISTYPE(&rXSDType, XSD::Types::ComplexType)) {
		const XSD::Types::ComplexType* pComplexType =
			dynamic_cast<const XSD::Types::ComplexType*>(&rXSDType);
		/* No type-level cycle guard here: structural recursion is bounded by
		   the per-element reference in ProcessElement, and derivation cycles
		   (extension/restriction base loops) by the guard in ProcessExtension.
		   A named type may legitimately recur through nested elements. */
		pComplexType->pValue_->ParseElement(*this);
	} else {
		/* inserts basic type. Handles array types the same as basic types */
		LuaType * pLuaType = dynamic_cast<LuaType*>(luaAdapter_());
		unique_ptr<LuaContent> pLuaContent(pLuaType->Content());
		unique_ptr<LuaType> pLeaf(pLuaContent->Type(rXSDType.Name(), 1));
		/* attach facets accumulated over the restriction chain, then reset */
		pLeaf->Facets(facets_);
		facets_.clear();
	}
}
