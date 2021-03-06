/*
 * Copyright 2007-2018 United States Government as represented by the
 * Administrator of The National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S. Code.
 * All Rights Reserved.
 */

#include <gmsec4/internal/mist/message/InternalMistMessage.h>

#include <gmsec4/internal/mist/MessageTemplate.h>
#include <gmsec4/internal/mist/FieldTemplate.h>
#include <gmsec4/internal/mist/SpecificationBuddy.h>
#include <gmsec4/internal/mist/InternalSpecification.h>

#include <gmsec4/internal/field/InternalField.h>

#include <gmsec4/internal/StringUtil.h>

#include <gmsec4/mist/Specification.h>
#include <gmsec4/mist/mist_defs.h>

#include <gmsec4/util/Log.h>

#include <limits>
#include <memory>
#include <sstream>

using namespace gmsec::api::internal;
using namespace gmsec::api::util;
using namespace gmsec::api::mist;
using namespace gmsec::api::mist::internal;


namespace gmsec
{
namespace api
{
namespace mist
{
namespace message
{
namespace internal
{

DataList<Field*> InternalMistMessage::m_standardFields;


InternalMistMessage::InternalMistMessage(const char* subject,
										 Message::MessageKind kind,
										 const char* schemaID,
										 const Specification& spec)
	: InternalMessage(subject, kind),
	  m_valid(false),
	  m_valueBuffer(),
	  m_spec(0),
	  m_template()
{
	std::auto_ptr<Specification> managedSpec(new Specification(spec));

	m_spec = managedSpec.get();

	registerTemplate(schemaID);

	managedSpec.release();

	init();
}


InternalMistMessage::InternalMistMessage(const char* subject,
										 Message::MessageKind kind,
										 const char* schemaID,
										 const gmsec::api::Config& config,
										 const Specification& spec)
	: InternalMessage(subject, kind, config),
	  m_valid(false),
	  m_valueBuffer(),
	  m_spec(0),
	  m_template()
{
	std::auto_ptr<Specification> managedSpec(new Specification(spec));

	m_spec = managedSpec.get();

	registerTemplate(schemaID);

	managedSpec.release();

	init();
}


InternalMistMessage::InternalMistMessage(const InternalMistMessage& other)
	: InternalMessage(other),
	  m_valid(false),
	  m_valueBuffer(),
	  m_spec(0),
	  m_template()
{
	std::auto_ptr<Specification> managedSpec(new Specification(*other.m_spec));

	m_spec = managedSpec.get();

	if(other.getTemplate().getID()!= NULL)
	{
		std::string id = other.getTemplate().getID();

		if(id != "")
		{
			m_template = MessageTemplate(other.getTemplate());
		}
	}

	managedSpec.release();

	init();
}


InternalMistMessage::InternalMistMessage(const char* data)
	: InternalMessage(data),
	  m_valid(false),
	  m_valueBuffer(),
	  m_spec(0),
	  m_template()
{
	unsigned int version = mist::GMSEC_ISD_CURRENT;

	try
	{
		version = (unsigned int) getUnsignedIntegerValue("CONTENT-VERSION") * 100;

		//TODO: We need a better way to handle ISD versions (and not just here, but throughout MIST code).

		if (version < mist::GMSEC_ISD_2014_00)
		{
			// 2014.00 is the oldest spec for which we have schemas/templates.
			GMSEC_DEBUG << "CONTENT-VERSION is " << version << "; defaulting to GMSEC_ISD_2014_00";
			version = mist::GMSEC_ISD_2014_00;
		}
		else if (version != mist::GMSEC_ISD_2014_00 && version != mist::GMSEC_ISD_2016_00)
		{
			GMSEC_WARNING << "Unknown value supplied with CONTENT-VERSION field (" << version << "); defaulting to GMSEC_ISD_CURRENT";
			version = mist::GMSEC_ISD_CURRENT;
		}
	}
	catch(...)
	{
		GMSEC_WARNING << "CONTENT-VERSION field not supplied, unable to determine specification version. Defaulting to GMSEC_ISD_CURRENT.";
	}

	std::ostringstream oss;
	oss << version;

	std::string schemaID = oss.str().insert(4, ".").append(".GMSEC");

	//determine message type (MSG, REQ, RESP)
	std::string type;
	try
	{
		type = getStringValue("MESSAGE-TYPE");
		schemaID.append(".").append(type);
	}
	catch(...)
	{
		throw Exception(MSG_ERROR, BAD_MSG_FORMAT, "MESSAGE-TYPE field not supplied, unable to determine message format");
	}

	//determine message subtype
	const char* subtype;
	try
	{
		subtype = getStringValue("MESSAGE-SUBTYPE");
		schemaID.append(".").append(subtype);
	}
	catch(...)
	{
		throw Exception(MSG_ERROR, BAD_MSG_FORMAT, "MESSAGE-SUBTYPE field not supplied, unable to determine message format");
	}

	// If we have made it here, then we can finally instantiate our Specification object
	// but we guard it using an std::auto_ptr in case other anomalies occur.
	std::auto_ptr<Specification> managedSpec(new Specification(Config()));

	m_spec = managedSpec.get();

	setSpecVersion(version);

	//determine if there is an additional subtype
	const std::list<SchemaTemplate>& directory = SpecificationBuddy::getInternal(*m_spec).getDirectory();

	for(std::list<SchemaTemplate>::const_iterator it = directory.begin(); it != directory.end(); ++it)
	{
		SchemaTemplate temp = *it;
		std::string name = type;
		name.append(".").append(subtype);
		if(StringUtil::stringEqualsIgnoreCase(name.c_str(), temp.getID()))
		{
			//we have the first two definitions already, we're interested in if there's a third definition
			temp.getFirstDefinition();
			temp.getNextDefinition();
			if(temp.hasNextDefinition())
			{
				schemaID.append(".").append(getStringValue(temp.getNextDefinition()));
			}
		}
	}

	registerTemplate(schemaID.c_str());

	managedSpec.release();

	init();
}


InternalMistMessage::~InternalMistMessage()
{
	delete m_spec;
}


void InternalMistMessage::registerTemplate(const char* schemaID)
{
	try
	{
		std::string id = schemaID;   // note: schemaID has already been verified to be non-NULL

		const MessageTemplate& temp = SpecificationBuddy::getInternal(*m_spec).findTemplate(id.c_str());

		//found the right template, now add the header fields
		std::list<FieldTemplate> fields = SpecificationBuddy::getInternal(*m_spec).prepHeaders(id.c_str());
		for(std::list<FieldTemplate>::const_iterator it = temp.listFieldTemplates().begin(); it != temp.listFieldTemplates().end(); ++it)
		{
			fields.push_back(*it);
		}

		m_template = MessageTemplate(id.c_str(), fields);
	}
	catch(Exception& e)
	{
		GMSEC_ERROR << "Failed to create MistMessage: schemaID given is not valid";
		throw Exception(e);
	}
}


bool InternalMistMessage::isValid() const
{
	return m_valid;
}


void InternalMistMessage::validate()
{
	m_valid = true;
}


void InternalMistMessage::invalidate()
{
	m_valid = false;
}


void InternalMistMessage::setStandardFields(const DataList<Field*>& standardFields)
{
	destroyStandardFields();

	for (DataList<Field*>::const_iterator it = standardFields.begin(); it != standardFields.end(); ++it)
	{
		const Field* field = *it;

		if (field)
		{
			m_standardFields.push_back(InternalField::makeFieldCopy(*field));
		}
		else
		{
			GMSEC_WARNING << "Field within DataList of standard fields is NULL";
		}
	}
}


void InternalMistMessage::clearStandardFields()
{
	destroyStandardFields();
}


void InternalMistMessage::destroyStandardFields()
{
	for (DataList<Field*>::iterator it = m_standardFields.begin(); it != m_standardFields.end(); ++it)
	{
		delete *it;
	}

	m_standardFields.clear();
}


const char* InternalMistMessage::getSchemaID() const
{
	std::string id = m_template.getID();
	if(id == "")
	{
		GMSEC_WARNING << getSubject() << " does not have an attached schema template";
	}
	return m_template.getID();
}


const MessageTemplate& InternalMistMessage::getTemplate() const
{
	return m_template;
}


void InternalMistMessage::setValue(const char* fieldName, const char* value)
{
	if(!fieldName || std::string(fieldName).empty())
	{
		throw Exception(MSG_ERROR, INVALID_FIELD_NAME, "Field name cannot be NULL or empty string");
	}
	if(!value)
	{
		throw Exception(MSG_ERROR, INVALID_FIELD_VALUE, "Field value cannot be NULL");
	}

	FieldTemplate fieldTemplate;

	try
	{//first find the template and see if it works
		fieldTemplate = findFieldTemplate(fieldName);
	}
	catch(...)
	{//if fieldTemplate does not exist, add StringField to the message
		addField(fieldName, value);

		return;
	}

	//found the template, now determine the type
	std::string type = fieldTemplate.getType();

	if(type == "CHAR")
	{//we can convert string to char, but we'll only accept it if it's one character long
		try
		{
			std::string str = value;
			if(str.size()==1)
			{//value can only be one character
				addField(fieldName, (GMSEC_CHAR) value[0]);
			}
			else
			{
				std::ostringstream err;
				err << "Template calls for field type GMSEC_CHAR, but string value \"" << value << "\" contains too many characters";
				throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
			}
		}
		catch(...)
		{
			std::ostringstream err;
			err << "Template calls for field type GMSEC_CHAR, but value \"" << value << "\" cannot be converted to GMSEC_CHAR";
			throw Exception(MSG_ERROR, INVALID_TYPE_CONVERSION, err.str().c_str());
		}
	}
	else if(type == "BOOLEAN")
	{//We can accept 4 values here: "0", "1", "true", "false"
		int intValue;
		StringUtil::STR2NUM_ERROR status = StringUtil::str2int(intValue, value);

		if(status == StringUtil::STR2NUM_SUCCESS)
		{//the value is indeed a number, but we can only accapt a 0 or 1
			if(intValue == 1)
			{
				addField(fieldName, true);
			}
			else if(intValue == 0)
			{
				addField(fieldName, false);
			}
			else
			{//it is not 0 or 1
				std::ostringstream err;
				err << "Template calls for field type GMSEC_BOOL, but value \"" << value << "\" cannot be converted to GMSEC_BOOL";
				throw Exception(MSG_ERROR, INVALID_TYPE_CONVERSION, err.str().c_str());
			}
		}
		else if(StringUtil::stringEqualsIgnoreCase(value, "true"))
		{//the value is not a number, so we check to see if it is "true"
			addField(fieldName, true);
		}
		else if(StringUtil::stringEqualsIgnoreCase(value, "false"))
		{//not "true", check if it is "false"
			addField(fieldName, false);
		}
		else
		{//this value is worthless, we can't do anything with it
			std::ostringstream err;
			err << "Template calls for field type GMSEC_BOOL, but value \"" << value << "\" cannot be converted to GMSEC_BOOL";
			throw Exception(MSG_ERROR, INVALID_TYPE_CONVERSION, err.str().c_str());
		}
	}
	else if(type == "I8")
	{
		int intValue;
		StringUtil::STR2NUM_ERROR status = StringUtil::str2int(intValue, value);
		
		if(status == StringUtil::STR2NUM_SUCCESS)
		{
			if(intValue >= int(std::numeric_limits<GMSEC_I8>::min()) && 
			   intValue <= int(std::numeric_limits<GMSEC_I8>::max()))
			{
				addField(fieldName, (GMSEC_I8) intValue);
			}
			else
			{//int is too big to fit into I8, which can lead to undefined behavior
				std::ostringstream err;
				err << "Template calls for field type GMSEC_I8, but value \"" << value << "\" is outside limits";
				throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
			}
		}
		else
		{
			std::ostringstream err;
			err << "Template calls for field type GMSEC_I8, but value \"" << value << "\" cannot be converted to GMSEC_I8";
			throw Exception(MSG_ERROR, INVALID_TYPE_CONVERSION, err.str().c_str());
		}
	}
	else if(type == "I16")
	{
		int intValue;
		StringUtil::STR2NUM_ERROR status = StringUtil::str2int(intValue, value);
		
		if(status == StringUtil::STR2NUM_SUCCESS)
		{
			if(intValue >= int(std::numeric_limits<GMSEC_I16>::min()) && 
			   intValue <= int(std::numeric_limits<GMSEC_I16>::max()))
			{
				addField(fieldName, (GMSEC_I16) intValue);
			}
			else
			{//int is too big to fit into I16, which can lead to undefined behavior
				std::ostringstream err;
				err << "Template calls for field type GMSEC_I16, but value \"" << value << "\" is outside limits";
				throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
			}
		}
		else
		{
			std::ostringstream err;
			err << "Template calls for field type GMSEC_I16, but value \"" << value << "\" cannot be converted to GMSEC_I16";
			throw Exception(MSG_ERROR, INVALID_TYPE_CONVERSION, err.str().c_str());
		}
	}
	else if(type == "I32")
	{
		int intValue;
		StringUtil::STR2NUM_ERROR status = StringUtil::str2int(intValue, value);
		
		if(status == StringUtil::STR2NUM_SUCCESS)
		{
			addField(fieldName, (GMSEC_I32) intValue);
		}
		else
		{
			std::ostringstream err;
			err << "Template calls for field type GMSEC_I32, but value \"" << value << "\" cannot be converted to GMSEC_I32";
			throw Exception(MSG_ERROR, INVALID_TYPE_CONVERSION, err.str().c_str());
		}
	}
	else if(type == "I64")
	{
		long long intValue;
		StringUtil::STR2NUM_ERROR status = StringUtil::str2longlong(intValue, value);
		
		if(status == StringUtil::STR2NUM_SUCCESS)
		{
			addField(fieldName, (GMSEC_I64) intValue);
		}
		else
		{
			std::ostringstream err;
			err << "Template calls for field type GMSEC_I64, but value \"" << value << "\" cannot be converted to GMSEC_I64";
			throw Exception(MSG_ERROR, INVALID_TYPE_CONVERSION, err.str().c_str());
		}
	}
	else if(type == "U8")
	{
		int intValue;
		StringUtil::STR2NUM_ERROR status = StringUtil::str2int(intValue, value);
		
		if(status == StringUtil::STR2NUM_SUCCESS)
		{
			if(intValue >= 0 && 
			   intValue <= int(std::numeric_limits<GMSEC_U8>::max()))
			{
				addField(fieldName, (GMSEC_U8) intValue);
			}
			else
			{//outside limits of a U8, which could produce undefined behavior
				std::ostringstream err;
				err << "Template calls for field type GMSEC_U8, but value \"" << value << "\" is outside limits";
				throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
			}
		}
		else
		{
			std::ostringstream err;
			err << "Template calls for field type GMSEC_U8, but value \"" << value << "\" cannot be converted to GMSEC_U8";
			throw Exception(MSG_ERROR, INVALID_TYPE_CONVERSION, err.str().c_str());
		}
	}
	else if(type == "U16")
	{
		int intValue;
		StringUtil::STR2NUM_ERROR status = StringUtil::str2int(intValue, value);
		
		if(status == StringUtil::STR2NUM_SUCCESS)
		{
			if(intValue >= 0 && 
			   intValue <= int(std::numeric_limits<GMSEC_U16>::max()))
			{
				addField(fieldName, (GMSEC_U16) intValue);
			}
			else
			{//outside limits of a U16, could produce undefined behavior
				std::ostringstream err;
				err << "Template calls for field type GMSEC_U16, but value \"" << value << "\" is outside limits";
				throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
			}
		}
		else
		{
			std::ostringstream err;
			err << "Template calls for field type GMSEC_U16, but value \"" << value << "\" cannot be converted to GMSEC_U16";
			throw Exception(MSG_ERROR, INVALID_TYPE_CONVERSION, err.str().c_str());
		}
	}
	else if(type == "U32")
	{
		int intValue;
		StringUtil::STR2NUM_ERROR status = StringUtil::str2int(intValue, value);
		
		if(status == StringUtil::STR2NUM_SUCCESS)
		{
			if(intValue >= 0 && 
			   intValue <= int(std::numeric_limits<GMSEC_U32>::max()))
			{
				addField(fieldName, (GMSEC_U32) intValue);
			}
			else
			{//outside limits
				std::ostringstream err;
				err << "Template calls for field type GMSEC_U32, but value \"" << value << "\" is outside limits";
				throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
			}
		}
		else
		{
			std::ostringstream err;
			err << "Template calls for field type GMSEC_U32, but value \"" << value << "\" cannot be converted to GMSEC_U32";
			throw Exception(MSG_ERROR, INVALID_TYPE_CONVERSION, err.str().c_str());
		}
	}
	else if(type == "U64")
	{
		long long intValue;
		StringUtil::STR2NUM_ERROR status = StringUtil::str2longlong(intValue, value);
		
		if(status == StringUtil::STR2NUM_SUCCESS)
		{
			if(intValue >=0)
			{
				addField(fieldName, (GMSEC_U64) intValue);
			}
			else
			{//intValue is a too negative and we don't need such toxicity in our lives
				std::ostringstream err;
				err << "Template calls for field type GMSEC_U64, but value \"" << value << "\" is outside limits";
				throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
			}
		}
		else
		{
			std::ostringstream err;
			err << "Template calls for field type GMSEC_U64, but value \"" << value << "\" cannot be converted to GMSEC_U64";
			throw Exception(MSG_ERROR, INVALID_TYPE_CONVERSION, err.str().c_str());
		}
	}
	else if(type == "F32")
	{
		GMSEC_F32 fValue;
		bool success = StringUtil::stringParseF32(value, fValue);
		if(success)
		{
			addField(fieldName, fValue);
		}
		else
		{
			std::ostringstream err;
			err << "Template calls for field type GMSEC_F32, but value \"" << value << "\" cannot be converted to GMSEC_F32";
			throw Exception(MSG_ERROR, INVALID_TYPE_CONVERSION, err.str().c_str());
		}
	}
	else if(type == "F64")
	{
		GMSEC_F64 fValue;
		bool success = StringUtil::stringParseF64(value, fValue);
		if(success)
		{
			addField(fieldName, fValue);
		}
		else
		{
			std::ostringstream err;
			err << "Template calls for field type GMSEC_F64, but value \"" << value << "\" cannot be converted to GMSEC_F64";
			throw Exception(MSG_ERROR, INVALID_TYPE_CONVERSION, err.str().c_str());
		}
	}
	else if(type == "STRING")
	{//a string value can very easily be converted to a string value
		addField(fieldName, value);
	}
	else if(type == "UNSET")
	{//field template has a variable data type, meaning it can be whatever it wants
	 //today, it's a StringField
		addField(fieldName, value);
	}
	else
	{//any types not listed can't be converted
			std::ostringstream err;
			err << "Template calls for field type " << type.c_str() << ", but value \"" << value << "\" cannot be converted to " << type.c_str();
			throw Exception(MSG_ERROR, INVALID_TYPE_CONVERSION, err.str().c_str());
	}
}

void InternalMistMessage::setValue(const char* fieldName, GMSEC_I64 value)
{
	if(!fieldName || std::string(fieldName).empty())
	{
		throw Exception(MSG_ERROR, INVALID_FIELD_NAME, "Field name cannot be NULL or empty string");
	}

	FieldTemplate fieldTemplate;

	try
	{//first try to find the fieldTemplate 
		fieldTemplate = findFieldTemplate(fieldName);
	}
	catch(...)
	{//fieldTemplate does not exist, so just add a new I64Field
		addField(fieldName, value);
	}

	//field template found, now determine the type
	std::string type = fieldTemplate.getType();

	if(type == "BOOLEAN")
	{//we'll only accept 0 or 1 as values
		if(value == 0)
		{
			addField(fieldName, false);
		}
		else if(value == 1)
		{
			addField(fieldName, true);
		}
		else
		{
			std::ostringstream err;
			err << "Template calls for field type GMSEC_BOOL, but value " << value << " cannot be converted to GMSEC_BOOL";
			throw Exception(MSG_ERROR, INVALID_TYPE_CONVERSION, err.str().c_str());
		}
	}
	else if(type == "I8")
	{
		if(value >= GMSEC_I64(std::numeric_limits<GMSEC_I8>::min()) && 
		   value <= GMSEC_I64(std::numeric_limits<GMSEC_I8>::max()))
		{
			addField(fieldName, (GMSEC_I8) value);
		}
		else
		{//can't cast because it's outside the limits
			std::ostringstream err;
			err << "Template calls for field type GMSEC_I8, but value " << value << " is outside limits";
			throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
		}
	}
	else if(type == "I16")
	{
		if(value >= GMSEC_I64(std::numeric_limits<GMSEC_I16>::min()) && 
		   value <= GMSEC_I64(std::numeric_limits<GMSEC_I16>::max()))
		{
			addField(fieldName, (GMSEC_I16) value);
		}
		else
		{//can't cast because it's outside the limits
			std::ostringstream err;
			err << "Template calls for field type GMSEC_I16, but value " << value << " is outside limits";
			throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
		}
	}
	else if(type == "I32")
	{
		if(value >= GMSEC_I64(std::numeric_limits<GMSEC_I32>::min()) && 
		   value <= GMSEC_I64(std::numeric_limits<GMSEC_I32>::max()))
		{
			addField(fieldName, (GMSEC_I32) value);
		}
		else
		{//can't cast because it's outside the limits
			std::ostringstream err;
			err << "Template calls for field type GMSEC_I32, but value " << value << " is outside limits";
			throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
		}
	}
	else if(type == "I64")
	{
		addField(fieldName, (GMSEC_I64) value);
	}
	else if(type == "U8")
	{
		if(value >= 0 && 
		   value <= GMSEC_I64(std::numeric_limits<GMSEC_U8>::max()))
		{
			addField(fieldName, (GMSEC_U8) value);
		}
		else
		{//can't cast because it's outside the limits
			std::ostringstream err;
			err << "Template calls for field type GMSEC_U8, but value " << value << " is outside limits";
			throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
		}
	}
	else if(type == "U16")
	{
		if(value >= 0 && 
		   value <= GMSEC_I64(std::numeric_limits<GMSEC_U16>::max()))
		{
			addField(fieldName, (GMSEC_U16) value);
		}
		else
		{//Tharr value be overflowin', Captain!
			std::ostringstream err;
			err << "Template calls for field type GMSEC_U16, but value " << value << " is outside limits";
			throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
		}
	}
	else if(type == "U32")
	{
		if(value >= 0 && 
		   value <= GMSEC_I64(std::numeric_limits<GMSEC_U32>::max()))
		{
			addField(fieldName, (GMSEC_U32) value);
		}
		else
		{//can't cast because it's outside the limits
			std::ostringstream err;
			err << "Template calls for field type GMSEC_U32, but value " << value << " is outside limits";
			throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
		}
	}
	else if(type == "U64")
	{
		if(value >= 0)
		{
			addField(fieldName, (GMSEC_U64) value);
		}
		else
		{//can't cast because it's outside the limits
			std::ostringstream err;
			err << "Template calls for field type GMSEC_U64, but value " << value << " is outside limits";
			throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
		}
	}
	else if(type == "F32")
	{
		addField(fieldName, (GMSEC_F32) value);
	}
	else if(type == "F64")
	{
		addField(fieldName, (GMSEC_F64) value);
	}
	else if(type == "STRING")
	{
		std::ostringstream oss;
		oss << value;
		addField(fieldName, oss.str().c_str());
	}
	else if(type == "UNSET")
	{//field template has a variable data type, meaning it can be whatever it wants
	 //today, it's an I64Field
		addField(fieldName, (GMSEC_I64) value);
	}
	else
	{//any types not listed can't be converted
			std::ostringstream err;
			err << "Template calls for field type " << type.c_str() << ", but value " << value << " cannot be converted to " << type.c_str();
			throw Exception(MSG_ERROR, INVALID_TYPE_CONVERSION, err.str().c_str());
	}
}

void InternalMistMessage::setValue(const char* fieldName, GMSEC_F64 value)
{
	if(!fieldName || std::string(fieldName).empty())
	{
		throw Exception(MSG_ERROR, INVALID_FIELD_NAME, "Field name cannot be NULL or empty string");
	}

	FieldTemplate fieldTemplate;

	try
	{//first try to find the fieldTemplate 
		fieldTemplate = findFieldTemplate(fieldName);
	}
	catch(...)
	{//fieldTemplate does not exist, so just add a new F64Field
		addField(fieldName, value);
	}

	//field template found, now determine the type
	std::string type = fieldTemplate.getType();

	if(type == "BOOLEAN")
	{//we'll only accept 0 or 1 as values
		//TODO: Fix this. Comparing F64 to an integer!
		if(value == 0)
		{
			addField(fieldName, false);
		}
		else if(value == 1)
		{
			addField(fieldName, true);
		}
		else
		{
			std::ostringstream err;
			err << "Template calls for field type GMSEC_BOOL, but value " << value << " cannot be converted to GMSEC_BOOL";
			throw Exception(MSG_ERROR, INVALID_TYPE_CONVERSION, err.str().c_str());
		}
	}
	else if(type == "I8")
	{
		if(value >= GMSEC_F64(std::numeric_limits<GMSEC_I8>::min()) && 
		   value <= GMSEC_F64(std::numeric_limits<GMSEC_I8>::max()))
		{
			GMSEC_WARNING << "conversion from 'GMSEC_F64' to 'GMSEC_I8', possible loss of data";
			addField(fieldName, (GMSEC_I8) value);
		}
		else
		{//can't cast because it's outside the limits
			std::ostringstream err;
			err << "Template calls for field type GMSEC_I8, but value " << value << " is outside limits";
			throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
		}
	}
	else if(type == "I16")
	{
		if(value >= GMSEC_F64(std::numeric_limits<GMSEC_I16>::min()) && 
		   value <= GMSEC_F64(std::numeric_limits<GMSEC_I16>::max()))
		{
			GMSEC_WARNING << "conversion from 'GMSEC_F64' to 'GMSEC_I16', possible loss of data";
			addField(fieldName, (GMSEC_I16) value);
		}
		else
		{//can't cast because it's outside the limits
			std::ostringstream err;
			err << "Template calls for field type GMSEC_I16, but value " << value << " is outside limits";
			throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
		}
	}
	else if(type == "I32")
	{
		if(value >= GMSEC_F64(std::numeric_limits<GMSEC_I32>::min()) && 
		   value <= GMSEC_F64(std::numeric_limits<GMSEC_I32>::max()))
		{
			GMSEC_WARNING << "conversion from 'GMSEC_F64' to 'GMSEC_I32', possible loss of data";
			addField(fieldName, (GMSEC_I32) value);
		}
		else
		{//can't cast because it's outside the limits
			std::ostringstream err;
			err << "Template calls for field type GMSEC_I32, but value " << value << " is outside limits";
			throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
		}
	}
	else if(type == "I64")
	{
		if(value >= GMSEC_F64(std::numeric_limits<GMSEC_I64>::min()) && 
		   value <= GMSEC_F64(std::numeric_limits<GMSEC_I64>::max()))
		{
			GMSEC_WARNING << "conversion from 'GMSEC_F64' to 'GMSEC_I64', possible loss of data";
			addField(fieldName, (GMSEC_I64) value);
		}
		else
		{//can't cast because it's outside the limits
			std::ostringstream err;
			err << "Template calls for field type GMSEC_I64, but value " << value << " is outside limits";
			throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
		}
	}
	else if(type == "U8")
	{
		if(value >= 0 && 
		   value <= GMSEC_F64(std::numeric_limits<GMSEC_U8>::max()))
		{
			GMSEC_WARNING << "conversion from 'GMSEC_F64' to 'GMSEC_U8', possible loss of data";
			addField(fieldName, (GMSEC_U8) value);
		}
		else
		{//can't cast because it's outside the limits
			std::ostringstream err;
			err << "Template calls for field type GMSEC_U8, but value " << value << " is outside limits";
			throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
		}
	}
	else if(type == "U16")
	{
		if(value >= 0 && 
		   value <= GMSEC_F64(std::numeric_limits<GMSEC_U16>::max()))
		{
			GMSEC_WARNING << "conversion from 'GMSEC_F64' to 'GMSEC_U16', possible loss of data";
			addField(fieldName, (GMSEC_U16) value);
		}
		else
		{//can't cast because it's outside the limits
			std::ostringstream err;
			err << "Template calls for field type GMSEC_U16, but value " << value << " is outside limits";
			throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
		}
	}
	else if(type == "U32")
	{
		if(value >= 0 && 
		   value <= GMSEC_F64(std::numeric_limits<GMSEC_U32>::max()))
		{
			GMSEC_WARNING << "conversion from 'GMSEC_F64' to 'GMSEC_U32', possible loss of data";
			addField(fieldName, (GMSEC_U32) value);
		}
		else
		{//can't cast because it's outside the limits
			std::ostringstream err;
			err << "Template calls for field type GMSEC_U32, but value " << value << " is outside limits";
			throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
		}
	}
	else if(type == "U64")
	{
		if(value >= 0 && 
		   value <= GMSEC_F64(std::numeric_limits<GMSEC_U64>::max()))
		{
			GMSEC_WARNING << "conversion from 'GMSEC_F64' to 'GMSEC_U64', possible loss of data";
			addField(fieldName, (GMSEC_U64) value);
		}
		else
		{//can't cast because it's outside the limits
			std::ostringstream err;
			err << "Template calls for field type GMSEC_U64, but value " << value << " is outside limits";
			throw Exception(MSG_ERROR, VALUE_OUT_OF_RANGE, err.str().c_str());
		}
	}
	else if(type == "F32")
	{
		addField(fieldName, (GMSEC_F32) value);
	}
	else if(type == "F64")
	{
		addField(fieldName, (GMSEC_F64) value);
	}
	else if(type == "STRING")
	{
		std::ostringstream oss;
		oss << value;
		addField(fieldName, oss.str().c_str());
	}
	else if(type == "UNSET")
	{//field template has a variable data type, meaning it can be whatever it wants
	 //today, it's an F64Field
		addField(fieldName, value);
	}
	else
	{//any types not listed can't be converted
			std::ostringstream err;
			err << "Template calls for field type " << type.c_str() << ", but value " << value << " cannot be converted to " << type.c_str();
			throw Exception(MSG_ERROR, INVALID_TYPE_CONVERSION, err.str().c_str());
	}
}

//size_t InternalMistMessage::findFieldTemplateIndex(const char* fieldName)
//{
//	const std::list<FieldTemplate>& fieldTemplates = m_template.listFieldTemplates();
//	
//	for(std::list<fieldTemplate>::const_iterator it = fieldTemplates.begin(); it != fieldTemplates.end(); ++it)
//	{
//		if(StringUtil::stringEquals(fieldName, it->getName()))
//		{
//			return i;
//		}
//	}
//
//	//if we have made it this far, it means we never found fieldName in m_template
//	//return -1 to indicate the FieldTemplate does not exist
//	return -1;
//
//}


const FieldTemplate& InternalMistMessage::findFieldTemplate(const char* fieldName)
{
	//nested array controls have their placeholder value added to the end of the list
	std::list<std::string> placeHolders;

	size_t arrayControlActive = 0;

	for(std::list<FieldTemplate>::const_iterator ft = m_template.listFieldTemplates().begin(); 
		ft != m_template.listFieldTemplates().end(); 
		++ft)
	{
		FieldTemplate temp = *ft;

		if(StringUtil::stringEqualsIgnoreCase(temp.getName().c_str(), "ARRAY-START") &&
		   StringUtil::stringEqualsIgnoreCase(temp.getMode().c_str(), "CONTROL"))
		{//we found an array control
		 //grab the placeholder value for the array control
			arrayControlActive++;
			placeHolders.push_back(temp.getValue());
		}
		else if(StringUtil::stringEqualsIgnoreCase(temp.getName().c_str(), "ARRAY-END") &&
				StringUtil::stringEqualsIgnoreCase(temp.getMode().c_str(), "CONTROL"))
		{//we can remove the last last value since we're done with it
			arrayControlActive--;
		}

		if(arrayControlActive>0)
		{//the names will need to be broken down into their elements and compared one by one, ignoring the index placeholders
			std::vector<std::string> fieldNameElements = StringUtil::split(fieldName, '.');
			std::vector<std::string> templateNameElements = StringUtil::split(temp.getName(), '.');
			bool found = false;

			//if they don't have the same amount of elements we can assume the names don't match
			if(fieldNameElements.size() == templateNameElements.size())
			{
				for(size_t i = 0; i<templateNameElements.size(); i++)
				{//compare each element of the field template's name at the specified index the field's element at the same index
					bool notPlaceHolder = true;

					for(std::list<std::string>::const_iterator placeHolder=placeHolders.begin(); placeHolder!=placeHolders.end(); ++placeHolder)
					{//check the current template element to make sure it isn't an index placeholder
						std::string value = *placeHolder;
						if(StringUtil::stringEquals(templateNameElements.at(i).c_str(), value.c_str()))
						{//it's an index placeholder, so the elements at this index won't be compared
							notPlaceHolder = false;
							break;
						}
					}

					if(notPlaceHolder)
					{//template element isn't an element, so compare the two elements at the index
						if(StringUtil::stringEquals(templateNameElements.at(i).c_str(), fieldNameElements.at(i).c_str()))
						{
							found = true;
						}
						else
						{
							found = false;
							break;
						}
					}
				}
			}
			if(found)
			{
				return *ft;
			}
		}
		else
		{//regular field
			if(StringUtil::stringEquals(temp.getName().c_str(), fieldName))
			{
				return *ft;
			}
		}
	}

	throw Exception(MIST_ERROR, FIELD_TEMPLATE_NOT_FOUND, "FieldTemplate not found in MessageTemplate");
}


std::string InternalMistMessage::buildSchemaID(Message::MessageKind kind, const char* type, const char* subtype, unsigned int version)
{
	//schema ID starts with version number, then add major.minor delimiter
	std::ostringstream oss;
	oss << version;
	std::string id = oss.str();

	//delimiter is inserted after major revision (a four digit number)
	id.insert(4, ".");

	//function only supports level-0 schema for the moment (the default schema as defined by the GMSEC ISD)
	id.append(".GMSEC");

	switch(kind)
	{
		case Message::PUBLISH:
			id.append(".MSG");
			break;
		case Message::REQUEST:
			id.append(".REQ");
			break;
		case Message::REPLY:
			id.append(".RESP");
			break;
	}

	if(type && !std::string(type).empty())
	{
		id.append(".");
		id.append(type);
	}

	if(subtype && !std::string(subtype).empty())
	{
		id.append(".");
		id.append(subtype);
	}

	return id;
}


Specification InternalMistMessage::buildSpecification(unsigned int version)
{
	std::ostringstream versionStr;
	versionStr << version;

	Config config;
	config.addValue("GMSEC-SPECIFICATION-VERSION", versionStr.str().c_str());

	return Specification(config);
}


Message::MessageKind InternalMistMessage::findKind(const char* schemaID, unsigned int version)
{
	if (!schemaID || std::string(schemaID).empty())
	{
		throw Exception(MSG_ERROR, UNKNOWN_MSG_TYPE, "SchemaID cannot be NULL nor contain an empty string");
	}

	// The types of Schema IDs that the API (currently) supports.
	//
	// Schema ID Type 1: major.minor.schemaLevel.messagekind.messagetype.<optionalmessagesubtype>
	// Schema ID Type 2: messagekind.messagetype.<optionalmessagesubtype>

	std::vector<std::string> elements = StringUtil::split(schemaID, '.');
	std::string kind;

	if (elements.size() >= 5)        // Type 1
	{
		kind = elements.at(3);
	}
	else if (elements.size() >= 2)   // Type 2
	{
		kind = elements.at(0);
	}

	if(StringUtil::stringEqualsIgnoreCase(kind.c_str(), "MSG"))
	{
		return Message::PUBLISH;
	}
	else if(StringUtil::stringEqualsIgnoreCase(kind.c_str(), "REQ"))
	{
		return Message::REQUEST;
	}
	else if(StringUtil::stringEqualsIgnoreCase(kind.c_str(), "RESP"))
	{
		return Message::REPLY;
	}
	else
	{
		std::ostringstream err;
		err << "MistMessage::findKind(): unable to determine the MessageKind \"" << kind.c_str() << "\"";
		throw Exception(MSG_ERROR, UNKNOWN_MSG_TYPE, err.str().c_str());
	}
}


unsigned int InternalMistMessage::getSpecVersion() const
{
	return m_spec->getVersion();
}

void InternalMistMessage::setSpecVersion(unsigned int version)
{
	SpecificationBuddy::getInternal(*m_spec).setVersion(version);
}

void InternalMistMessage::init()
{	
	const std::list<FieldTemplate>& fieldTemplates = getTemplate().listFieldTemplates();

	for(std::list<FieldTemplate>::const_iterator it = fieldTemplates.begin(); it != fieldTemplates.end(); ++it)
	{
		FieldTemplate temp = *it;

		if(temp.hasExplicitType() && temp.hasExplicitValue())
		{//the field template has an explicitly defined type and value, so we'll add the field to the message
			try
			{
				std::auto_ptr<Field> field(temp.toField(temp.getType()));

				addField(*(field.get()));
			}
			catch(...)
			{
				//ignore control fields, do nothing
			}
		}
	}

	// Add standard fields (if any) to the message
	//
	for (DataList<Field*>::const_iterator it = m_standardFields.begin(); it != m_standardFields.end(); ++it)
	{
		const Field* field = *it;

		addField(*field);
	}
}

} // namespace internal
} // namespace message
} // namespace mist
} // namespace api
} // namespace gmsec
