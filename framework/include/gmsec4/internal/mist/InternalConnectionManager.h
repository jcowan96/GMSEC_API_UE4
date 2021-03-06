/*
 * Copyright 2007-2018 United States Government as represented by the
 * Administrator of The National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S. Code.
 * All Rights Reserved.
 */



/** @file InternalConnectionManager.h
 *
**/

#ifndef GMSEC_INTERNAL_CONNECTIONMANAGER_H
#define GMSEC_INTERNAL_CONNECTIONMANAGER_H

#include <gmsec4/Config.h>
#include <gmsec4/Connection.h>
#include <gmsec4/Message.h>
#include <gmsec4/Status.h>

#include <gmsec4/internal/mist/HeartbeatService.h>
#include <gmsec4/internal/mist/MistCallbackAdapter.h>
#include <gmsec4/internal/mist/ResourceService.h>

#include <gmsec4/internal/mist/MessagePopulator.h>

#include <gmsec4/util/DataList.h>

#include <gmsec4/util/StdSharedPtr.h>
#include <gmsec4/util/Mutex.h>
#include <gmsec4/util/StdThread.h>

#include <gmsec4/util/wdllexp.h>

#include <gmsec4_defs.h>

#include <list>
#include <memory>
#include <string>


namespace gmsec
{
namespace api
{
	// Forward declarations(s)
	class Field;

namespace mist
{
	// Forward declarations(s)
	class ConnectionManager;
	class ConnectionManagerCallback;
	class ConnectionManagerEventCallback;
	class ConnectionManagerReplyCallback;
	class Specification;
	class SubscriptionInfo;

namespace internal
{
	// Forward declarations(s)
	class CustomSpecification;


/** @class InternalConnectionManager
*/
class GMSEC_API InternalConnectionManager
{
public:
	static const char* CALL_TYPE getAPIVersion();


	InternalConnectionManager(gmsec::api::mist::ConnectionManager* parent, const Config& cfg, bool validate, unsigned int version);


	~InternalConnectionManager();


	void CALL_TYPE initialize(); 


	void CALL_TYPE cleanup();


	Connection::ConnectionState CALL_TYPE getState() const;


	const char* CALL_TYPE getLibraryRootName() const;


	const char* CALL_TYPE getLibraryVersion() const;


	Specification& CALL_TYPE getSpecification() const;


	void CALL_TYPE setSpecification(Specification* spec);


	void CALL_TYPE setStandardFields(const gmsec::api::util::DataList<Field*>& standardFields);


	const gmsec::api::util::DataList<Field*>& CALL_TYPE getStandardFields() const;


	void CALL_TYPE addStandardFields(Message& msg) const;


	void CALL_TYPE registerEventCallback(Connection::ConnectionEvent event, ConnectionManagerEventCallback* cb);


	SubscriptionInfo* CALL_TYPE subscribe(const char *subject, ConnectionManagerCallback* cb = NULL);


	SubscriptionInfo* CALL_TYPE subscribe(const char* subject, const Config& config, ConnectionManagerCallback* cb = NULL);


	void CALL_TYPE unsubscribe(SubscriptionInfo*& info);


	void CALL_TYPE publish(const Message& msg);


	void CALL_TYPE publish(const Message& msg, const Config &config);


	void CALL_TYPE request(const Message& request, GMSEC_I32 timeout, ConnectionManagerReplyCallback* cb, GMSEC_I32 republish_ms = 0);


	Message* CALL_TYPE request(const Message& request, GMSEC_I32 timeout, GMSEC_I32 republish_ms = 0);


	void CALL_TYPE cancelRequest(ConnectionManagerReplyCallback* cb);


	void CALL_TYPE reply(const Message& request, const Message& reply);


	void CALL_TYPE dispatch(const Message& msg);


	Message* CALL_TYPE receive(GMSEC_I32 timeout);


	void CALL_TYPE release(Message*& msg);


	bool CALL_TYPE startAutoDispatch();


	bool CALL_TYPE stopAutoDispatch(bool waitForCompletion = true);


	void CALL_TYPE excludeSubject(const char* subject);


	void CALL_TYPE removeExcludedSubject(const char* subject);


	const char* CALL_TYPE getName() const;


	void CALL_TYPE setName(const char* name);


	const char* CALL_TYPE getID() const;


	const char* CALL_TYPE getMWInfo() const;


	GMSEC_U64 CALL_TYPE getPublishQueueMessageCount() const;


	Message CALL_TYPE createHeartbeatMessage(const char* subject, const gmsec::api::util::DataList<Field*>& heartbeatFields);


	void CALL_TYPE startHeartbeatService(const char* subject, const gmsec::api::util::DataList<Field*>& heartbeatFields);


	void CALL_TYPE stopHeartbeatService();


	Status CALL_TYPE setHeartbeatServiceField(const Field& field);


	Message CALL_TYPE createLogMessage(const char* subject, const gmsec::api::util::DataList<Field*>& logFields);


	void CALL_TYPE setLoggingDefaults(const char* subject, const gmsec::api::util::DataList<Field*>& logFields);


	void CALL_TYPE publishLog(const char* msg, const Field& severity);


	void CALL_TYPE acknowledgeSimpleService(const char* subject,
	                                        const Message& request,
	                                        ResponseStatus::Response ssResponse,
	                                        const gmsec::api::util::DataList<Field*>& fields);


	void CALL_TYPE requestSimpleService(const char* subject,
	                                    const char* opName,
	                                    const Field& opNumber,
	                                    const gmsec::api::util::DataList<Field*>& fields,
	                                    const gmsec::api::util::DataList<ServiceParam*>& sParams);


	void CALL_TYPE requestSimpleService(const char* subject,
	                                    const char* opName,
	                                    const Field& opNumber,
	                                    const gmsec::api::util::DataList<Field*>& fields,
	                                    const gmsec::api::util::DataList<ServiceParam*>& sParams,
	                                    GMSEC_I32 timeout,
	                                    ConnectionManagerReplyCallback* cb,
	                                    GMSEC_I32 republish_ms = 0);


	Message* CALL_TYPE requestSimpleService(const char* subject,
	                                        const char* opName,
	                                        const Field& opNumber,
	                                        const gmsec::api::util::DataList<Field*>& fields,
	                                        const gmsec::api::util::DataList<ServiceParam*>& sParams,
	                                        GMSEC_I32 timeout,
	                                        GMSEC_I32 republish_ms = 0);


	void publishResourceMessage(const char* subject, size_t sampleInterval, size_t averageInterval);


	Message createResourceMessage(const char* subject, size_t sampleInterval, size_t averageInterval);


	void startResourceMessageService(const char* subject, size_t intervalSeconds, size_t sampleInterval, size_t averageInterval);


	bool stopResourceMessageService();


	void CALL_TYPE requestDirective(const char* subject, const Field& directiveString, const gmsec::api::util::DataList<Field*>& fields);

		
	void CALL_TYPE requestDirective(const char* subject,
	                                const Field& directiveString,
	                                const gmsec::api::util::DataList<Field*>& fields,
	                                GMSEC_I32 timeout,
	                                ConnectionManagerReplyCallback* cb,
	                                GMSEC_I32 republish_ms = 0);

	
	Message* CALL_TYPE requestDirective(const char* subject,
	                                    const Field& directiveString,
	                                    const gmsec::api::util::DataList<Field*>& fields,
	                                    GMSEC_I32 timeout,
	                                    GMSEC_I32 republish_ms = 0);


	void CALL_TYPE acknowledgeDirectiveRequest(const char* subject,
	                                           const Message& request,
	                                           ResponseStatus::Response ssResponse,
	                                           const gmsec::api::util::DataList<Field*>& fields);


	static void shutdownAllMiddlewares();


    /* For C-binding only! */
	void CALL_TYPE setSpecification(Specification* spec, GMSEC_SpecificationValidateMessage* validateMsg);


    /* For C-binding only! */
    void CALL_TYPE registerEventCallback(Connection::ConnectionEvent event, GMSEC_ConnectionMgrEventCallback* ecb);


    /* For C-binding only! */
    SubscriptionInfo* CALL_TYPE subscribe(const char* subject, const Config& cfg, GMSEC_ConnectionMgrCallback* cb);


    /* For C-binding only! */
    void CALL_TYPE request(const Message& req,
	                       GMSEC_I32 timeout,
	                       GMSEC_ConnectionMgrReplyCallback* rcb,
	                       GMSEC_ConnectionMgrEventCallback* ecb,
	                       GMSEC_I32 republish_ms);


    /* For C-binding only! */
    void CALL_TYPE cancelRequest(GMSEC_ConnectionMgrReplyCallback* rcb);


    /* For C-binding only! */
    void CALL_TYPE requestDirective(const char* subject,
	                                const Field& directiveString,
	                                const gmsec::api::util::DataList<Field*>& fields,
	                                GMSEC_I32 timeout,
	                                GMSEC_ConnectionMgrReplyCallback* rcb,
	                                GMSEC_ConnectionMgrEventCallback* ecb,
	                                GMSEC_I32 republish_ms);


    /* For C-binding only! */
    void CALL_TYPE requestSimpleService(const char* subject,
	                                    const char* opName,
	                                    const Field& opNumber,
	                                    const gmsec::api::util::DataList<Field*>& fields,
	                                    const gmsec::api::util::DataList<ServiceParam*>& params,
	                                    GMSEC_I32 timeout,
	                                    GMSEC_ConnectionMgrReplyCallback* rcb,
	                                    GMSEC_ConnectionMgrEventCallback* ecb,
	                                    GMSEC_I32 republish_ms);


private:
	typedef gmsec::api::util::DataList<Field*> FieldList;

	// Defined, but not implemented
	InternalConnectionManager();
	InternalConnectionManager(const InternalConnectionManager &);
	InternalConnectionManager &operator=(const InternalConnectionManager &);


	// helper functions
	inline bool validateOnSend() { return m_validate == VALIDATE_ALL || m_validate == VALIDATE_SEND; }

	inline bool validateOnRecv() { return m_validate == VALIDATE_ALL || m_validate == VALIDATE_RECV; }

	void setHeartbeatDefaults(const char* subject, const gmsec::api::util::DataList<Field*>& heartbeatFields);

	Message createMessage(const char* subject, Message::MessageKind kind, const FieldList& fields);

	void destroyFields(FieldList& flist);


	typedef std::list<SubscriptionInfo*> SubscriptionList;

	enum ValidateLevel
	{
		VALIDATE_NONE,
		VALIDATE_RECV,
		VALIDATE_SEND,
		VALIDATE_ALL
	};

	// member data
	Config                               m_config;
	Connection*                          m_connection;
	ValidateLevel                        m_validate;
	Specification*                       m_specification;
	Specification*                       m_customSpecification;
	FieldList                            m_standardHeartbeatFields;
	FieldList                            m_standardLogFields;
	std::string                          m_heartbeatSubject;
	std::string                          m_logSubject;
	size_t                               m_resourceMessageCounter;
	SubscriptionList                     m_subscriptions;
	MessagePopulator*                    m_messagePopulator;
	mutable gmsec::api::util::Mutex      m_cmLock;

	// will be used for support of Callbacks
	gmsec::api::mist::ConnectionManager* m_parent;
	MistCallbackAdapter*                 m_callbackAdapter;

	std::auto_ptr<gmsec::api::util::StdThread>       m_hbThread;
	gmsec::api::util::StdSharedPtr<HeartbeatService> m_hbService;

	std::auto_ptr<gmsec::api::util::StdThread>       m_rsrcThread;
	gmsec::api::util::StdSharedPtr<ResourceService>  m_rsrcService;

	// will be used for support of C binding
	CustomSpecification* m_ceeCustomSpec;
};

}  //namespace internal
}  //namespace mist
}  //namespace api
}  //namespace gmsec

#endif
