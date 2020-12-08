/*
 * FreeRTOS V202011.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://aws.amazon.com/freertos
 *
 */

/**
 * @file mqtt_agent.h
 * @brief Functions for running a coreMQTT client in a dedicated thread.
 */
#ifndef MQTT_AGENT_H
#define MQTT_AGENT_H

/* Demo Specific configs. */
#include "demo_config.h"

/* MQTT library includes. */
#include "core_mqtt.h"
#include "core_mqtt_state.h"


/**
 * @brief The size to use for the network buffer.
 */
#ifndef mqttexampleNETWORK_BUFFER_SIZE
    #define mqttexampleNETWORK_BUFFER_SIZE    ( 1024U )
#endif

/**
 * @brief Timeout for MQTT_ProcessLoop function in milliseconds.
 *
 * This demo uses no delay for the process loop, so each invocation will run
 * one iteration, and will only receive a single packet. However, if there is
 * no data available on the socket, the entire socket timeout value will elapse.
 */
#define MQTT_AGENT_PROCESS_LOOP_TIMEOUT_MS     ( 0U )

/**
 * @brief The maximum number of MQTT connections that can be tracked.
 */
#define MAX_CONNECTIONS                        2

/**
 * @brief The maximum number of pending acknowledgments to track for a single
 * connection.
 */
#define PENDING_ACKS_MAX_SIZE                  20

/**
 * @brief The maximum number of subscriptions to track for a single connection.
 */
#define SUBSCRIPTIONS_MAX_COUNT                10

/**
 * @brief Size of statically allocated buffers for holding subscription filters.
 */
#define MQTT_AGENT_SUBSCRIPTION_BUFFER_SIZE    100

/**
 * @brief Ticks to wait for task notifications.
 */
#define MQTT_AGENT_QUEUE_WAIT_TIME             pdMS_TO_TICKS( 1000 )

/*-----------------------------------------------------------*/

/**
 * @brief Struct containing context for a specific command.
 *
 * @note An instance of this struct and any variables it points to MUST stay
 * in scope until the associated command is processed, and its callback called.
 */
struct CommandContext;
typedef struct CommandContext CommandContext_t;

/**
 * @brief Callback function called when a command completes.
 */
typedef void (* CommandCallback_t )( void *,
                                     MQTTStatus_t );

/**
 * @brief Callback function called when a publish is received.
 */
typedef void (* PublishCallback_t )( MQTTPublishInfo_t * pxPublishInfo,
                                     void * pxSubscriptionContext );

/**
 * @brief MQTT contexts are owned by the MQTT agent and referenced using handles of
 * type.
 */
typedef int MQTTContextHandle_t;

/*-----------------------------------------------------------*/

/**
 * @brief Process commands from the command queue in a loop.
 *
 * This demo requires a process loop command to be enqueued before calling this
 * function, and will re-add a process loop command every time one is processed.
 * This demo will exit the loop after receiving an unsubscribe operation.
 *
 * @return pointer to MQTT context that caused error, or `NULL` if terminated
 * gracefully.
 */
MQTTContext_t * MQTTAgent_CommandLoop( void );

/**
 * @brief Resume a session by resending publishes if a session is present in
 * the broker, or reestablish subscriptions if not.
 *
 * @param[in] mqttContextHandle Handle to the MQTT connection to resume.
 * @param[in] sessionPresent The session present flag from the broker.
 *
 * @return `MQTTSuccess` if it succeeds in resending publishes, else an
 * appropriate error code from `MQTT_Publish()`
 */
MQTTStatus_t MQTTAgent_ResumeSession( MQTTContextHandle_t mqttContextHandle,
                                      bool sessionPresent );

/**
 * @brief Add a command to call MQTT_Subscribe() for an MQTT connection.
 *
 * @param[in] mqttContextHandle Handle to the MQTT connection to use.
 * @param[in] pSubscriptionInfo Struct describing topic to subscribe to.
 * @param[in] incomingPublishCallback Incoming publish callback for the subscriptions.
 * @param[in] incomingPublishCallbackContext Context for the publish callback.
 * @param[in] commandCompleteCallback Optional callback to invoke when the command completes.
 * @param[in] commandCompleteCallbackContext Optional completion callback context.
 *
 * @return `true` if the command was enqueued, else `false`.
 */
bool MQTTAgent_Subscribe( MQTTContextHandle_t mqttContextHandle,
                          MQTTSubscribeInfo_t * pSubscriptionInfo,
                          PublishCallback_t incomingPublishCallback,
                          void * incomingPublishCallbackContext,
                          CommandCallback_t commandCompleteCallback,
                          void * commandCompleteCallbackContext );
/**
 * @brief Add a command to call MQTT_Unsubscribe() for an MQTT connection.
 *
 * @param[in] mqttContextHandle Handle to the MQTT connection to use.
 * @param[in] pSubscriptionList List of topics to unsubscribe from.
 * @param[in] pCommandCompleteCallbackContext Optional completion callback context.
 * @param[in] cmdCompleteCallback Optional callback to invoke when the command completes.
 *
 * @return `true` if the command was enqueued, else `false`.
 */
bool MQTTAgent_Unsubscribe( MQTTContextHandle_t mqttContextHandle,
                            MQTTSubscribeInfo_t * pSubscriptionList,
                            CommandContext_t * pCommandCompleteCallbackContext,
                            CommandCallback_t cmdCompleteCallback );

/**
 * @brief Add a command to call MQTT_Publish() for an MQTT connection.
 *
 * @param[in] mqttContextHandle Handle for the MQTT context to use.
 * @param[in] pPublishInfo MQTT PUBLISH information.
 * @param[in] commandCompleteCallback Optional callback to invoke when the command completes.
 * @param[in] commandCompleteCallbackContext Optional completion callback context.
 *
 * @return `true` if the command was enqueued, else `false`.
 */
bool MQTTAgent_Publish( MQTTContextHandle_t mqttContextHandle,
                        MQTTPublishInfo_t * pPublishInfo,
                        CommandCallback_t commandCompleteCallback,
                        CommandContext_t * commandCompleteCallbackContext );

/**
 * @brief Add a command to call MQTT_ProcessLoop() for an MQTT connection.
 *
 * @param[in] mqttContextHandle Handle of the MQTT connection to use.
 * @param[in] timeoutMs Timeout for MQTT_ProcessLoop().
 * @param[in] pCommandCompleteCallbackContext Optional completion callback context.
 * @param[in] cmdCompleteCallback Optional callback to invoke when the command completes.
 *
 * @return `true` if the command was enqueued, else `false`.
 */
bool MQTTAgent_ProcessLoop( MQTTContextHandle_t mqttContextHandle,
                            CommandContext_t * pCommandCompleteCallbackContext,
                            CommandCallback_t cmdCompleteCallback );

/**
 * @brief Add a command to call MQTT_Ping() for an MQTT connection.
 *
 * @param[in] mqttContextHandle Handle of the MQTT connection to use.
 * @param[in] pCommandCompleteCallbackContext Optional completion callback context.
 * @param[in] cmdCompleteCallback Optional callback to invoke when the command completes.
 *
 * @return `true` if the command was enqueued, else `false`.
 */
bool MQTTAgent_Ping( MQTTContextHandle_t mqttContextHandle,
                     CommandContext_t * pCommandCompleteCallbackContext,
                     CommandCallback_t cmdCompleteCallback );

/**
 * @brief Add a command to disconnect an MQTT connection.
 *
 * @param[in] mqttContextHandle Handle of the MQTT connection to use.
 * @param[in] pCommandCompleteCallbackContext Optional completion callback context.
 * @param[in] cmdCompleteCallback Optional callback to invoke when the command completes.
 *
 * @return `true` if the command was enqueued, else `false`.
 */
bool MQTTAgent_Disconnect( MQTTContextHandle_t mqttContextHandle,
                           CommandContext_t * pCommandCompleteCallbackContext,
                           CommandCallback_t cmdCompleteCallback );

/**
 * @brief Add a command to clear memory associated with an MQTT connection.
 *
 * @param[in] mqttContextHandle Handle of the MQTT context to clear.
 * @param[in] pCommandCompleteCallbackContext Optional completion callback context.
 * @param[in] cmdCompleteCallback Optional callback to invoke when the command completes.
 *
 * @return `true` if the command was enqueued, else `false`.
 */
bool MQTTAgent_Free( MQTTContextHandle_t mqttContextHandle,
                     CommandContext_t * pCommandCompleteCallbackContext,
                     CommandCallback_t cmdCompleteCallback );

/**
 * @brief Add a termination command to the command queue.
 *
 * @return `true` if the command was enqueued, else `false`.
 */
bool MQTTAgent_Terminate( void );

/**
 * @brief Get the number of commands waiting in the queue.
 *
 * @return The number of enqueued commands.
 */
uint32_t MQTTAgent_GetNumWaiting( void );

/**
 * @brief Perform any initialisation the MQTT agent requires before it is can
 * be used.  Must called before any other function.
 *
 * @param[in] mqttContextHandle Handle of the first MQTT context to use with the
 * agent.
 * @param[in] pTransportInterface Transport interface to use with the MQTT.
 * library.  See https://www.freertos.org/network-interface.html
 * @param[in] getCurrentTimeMs Pointer to a function that returns a count value
 * that increments every millisecond.
 * @param[in] unkownIncomingPublishCallback A callback to execute should the
 * agent receive a publish message from a topic filter it is not subscribed to.
 * This can happen with incoming control information.
 * @param[in] pDefaultPublishContext A pointer to a context structure defined by
 * the application writer.  The context is passed into
 * unkownIncomingPublishCallback() should it be called.
 *
 * @return `true` if the command was enqueued, else `false`.
 */
MQTTStatus_t MQTTAgent_Init( MQTTContextHandle_t mqttContextHandle,
                             TransportInterface_t *pTransportInterface,
                             MQTTGetCurrentTimeFunc_t getCurrentTimeMs,
                             PublishCallback_t unkownIncomingPublishCallback,
                             void * pDefaultPublishContext );

/**
 * @brief Connects to an MQTT broker.  Note this function uses the transport
 * interface passed in using MQTTAgent_Init().  This function only creates the
 * MQTT connection, it does not create the TCP connection.  It also calls
 * the coreMQTT_Connect() API directly, not from within the context of the MQTT
 * agent task.
 *
 * @param[in] mqttContextHandle Handle of the MQTT context that should connect to
 * the broker.
 * @param[in] pConnectInfo Pointer to a structure that describes the connection
 * to make.
 * @param[in] pWillInfo Pointer to a structure that describes the MQTT Last Will
 * and Testament message associated with this connection.  See the MQTT
 * specification.
 * @param[in] timeoutMs The maximum time in milliseconds to wait for a connection
 * to be established before giving up.
 * @param[out] pSessionPresent Whether a previous session was present. Only
 * relevant if not establishing a clean session.
 * unkownIncomingPublishCallback() should it be called.
 *
 * @return #MQTTNoMemory if the #MQTTContext_t.networkBuffer is too small to
 * hold the MQTT packet;
 * #MQTTBadParameter if invalid parameters are passed;
 * #MQTTSendFailed if transport send failed;
 * #MQTTRecvFailed if transport receive failed for CONNACK;
 * #MQTTNoDataAvailable if no data available to receive in transport until
 * the @p timeoutMs for CONNACK;
 * #MQTTSuccess otherwise.
 */
MQTTStatus_t MQTTAgent_Connect( MQTTContextHandle_t mqttContextHandle,
                                const MQTTConnectInfo_t * pConnectInfo,
                                const MQTTPublishInfo_t * pWillInfo,
                                uint32_t timeoutMs,
                                bool * pSessionPresent );


#endif /* MQTT_AGENT_H */
