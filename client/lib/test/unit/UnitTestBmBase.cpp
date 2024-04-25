#include "UnitTestBmBase.h"

#include "bm_client.h"

const UnitTestBmBase::LibFuncs& UnitTestBmBase::getFuncs()
{
    static LibFuncs funcs;
    funcs.m_alloc = &cc_mqtt311_bm_client_alloc;
    funcs.m_free = &cc_mqtt311_bm_client_free;
    funcs.m_tick = &cc_mqtt311_bm_client_tick;
    funcs.m_process_data = &cc_mqtt311_bm_client_process_data;
    funcs.m_notify_network_disconnected = &cc_mqtt311_bm_client_notify_network_disconnected;
    funcs.m_is_network_disconnected = &cc_mqtt311_bm_client_is_network_disconnected;
    funcs.m_set_default_response_timeout = &cc_mqtt311_bm_client_set_default_response_timeout;
    funcs.m_get_default_response_timeout = &cc_mqtt311_bm_client_get_default_response_timeout;
    funcs.m_set_verify_outgoing_topic_enabled = &cc_mqtt311_bm_client_set_verify_outgoing_topic_enabled;
    funcs.m_get_verify_outgoing_topic_enabled = &cc_mqtt311_bm_client_get_verify_outgoing_topic_enabled;
    funcs.m_set_verify_incoming_topic_enabled = &cc_mqtt311_bm_client_set_verify_incoming_topic_enabled;
    funcs.m_get_verify_incoming_topic_enabled = &cc_mqtt311_bm_client_get_verify_incoming_topic_enabled;
    funcs.m_set_verify_incoming_msg_subscribed = &cc_mqtt311_bm_client_set_verify_incoming_msg_subscribed;
    funcs.m_get_verify_incoming_msg_subscribed = &cc_mqtt311_bm_client_get_verify_incoming_msg_subscribed;
    funcs.m_connect_prepare = &cc_mqtt311_bm_client_connect_prepare;
    funcs.m_connect_init_config = &cc_mqtt311_bm_client_connect_init_config;
    funcs.m_connect_init_config_will = &cc_mqtt311_bm_client_connect_init_config_will;
    funcs.m_connect_set_response_timeout = &cc_mqtt311_bm_client_connect_set_response_timeout;
    funcs.m_connect_get_response_timeout = &cc_mqtt311_bm_client_connect_get_response_timeout;
    funcs.m_connect_config = &cc_mqtt311_bm_client_connect_config;
    funcs.m_connect_config_will = &cc_mqtt311_bm_client_connect_config_will;
    funcs.m_connect_send = &cc_mqtt311_bm_client_connect_send;
    funcs.m_connect_cancel = &cc_mqtt311_bm_client_connect_cancel;
    funcs.m_connect = &cc_mqtt311_bm_client_connect;
    funcs.m_is_connected = &cc_mqtt311_bm_client_is_connected;
    funcs.m_disconnect_prepare = &cc_mqtt311_bm_client_disconnect_prepare;
    funcs.m_disconnect_send = &cc_mqtt311_bm_client_disconnect_send;
    funcs.m_disconnect_cancel = &cc_mqtt311_bm_client_disconnect_cancel;
    funcs.m_disconnect = &cc_mqtt311_bm_client_disconnect;
    funcs.m_subscribe_prepare = &cc_mqtt311_bm_client_subscribe_prepare;
    funcs.m_subscribe_set_response_timeout = &cc_mqtt311_bm_client_subscribe_set_response_timeout;
    funcs.m_subscribe_get_response_timeout = &cc_mqtt311_bm_client_subscribe_get_response_timeout;
    funcs.m_subscribe_init_config_topic = &cc_mqtt311_bm_client_subscribe_init_config_topic;
    funcs.m_subscribe_config_topic = &cc_mqtt311_bm_client_subscribe_config_topic;
    funcs.m_subscribe_send = &cc_mqtt311_bm_client_subscribe_send;
    funcs.m_subscribe_cancel = &cc_mqtt311_bm_client_subscribe_cancel;
    funcs.m_subscribe = &cc_mqtt311_bm_client_subscribe;
    funcs.m_unsubscribe_prepare = &cc_mqtt311_bm_client_unsubscribe_prepare;
    funcs.m_unsubscribe_set_response_timeout = &cc_mqtt311_bm_client_unsubscribe_set_response_timeout;
    funcs.m_unsubscribe_get_response_timeout = &cc_mqtt311_bm_client_unsubscribe_get_response_timeout;
    funcs.m_unsubscribe_init_config_topic = &cc_mqtt311_bm_client_unsubscribe_init_config_topic;
    funcs.m_unsubscribe_config_topic = &cc_mqtt311_bm_client_unsubscribe_config_topic;
    funcs.m_unsubscribe_send = &cc_mqtt311_bm_client_unsubscribe_send;
    funcs.m_unsubscribe_cancel = &cc_mqtt311_bm_client_unsubscribe_cancel;    
    funcs.m_unsubscribe = &cc_mqtt311_bm_client_unsubscribe;
    funcs.m_publish_prepare = &cc_mqtt311_bm_client_publish_prepare;    
    funcs.m_publish_count = &cc_mqtt311_bm_client_publish_count;    
    funcs.m_publish_init_config = &cc_mqtt311_bm_client_publish_init_config;
    funcs.m_publish_set_response_timeout = &cc_mqtt311_bm_client_publish_set_response_timeout;
    funcs.m_publish_get_response_timeout = &cc_mqtt311_bm_client_publish_get_response_timeout;
    funcs.m_publish_set_resend_attempts = &cc_mqtt311_bm_client_publish_set_resend_attempts;
    funcs.m_publish_get_resend_attempts = &cc_mqtt311_bm_client_publish_get_resend_attempts;
    funcs.m_publish_config = &cc_mqtt311_bm_client_publish_config;
    funcs.m_publish_send = &cc_mqtt311_bm_client_publish_send;
    funcs.m_publish_cancel = &cc_mqtt311_bm_client_publish_cancel;    
    funcs.m_publish_was_initiated = &cc_mqtt311_bm_client_publish_was_initiated;    
    funcs.m_publish = &cc_mqtt311_bm_client_publish;    
    funcs.m_publish_set_ordering = &cc_mqtt311_bm_client_publish_set_ordering;
    funcs.m_publish_get_ordering = &cc_mqtt311_bm_client_publish_get_ordering;
    funcs.m_set_next_tick_program_callback = &cc_mqtt311_bm_client_set_next_tick_program_callback;
    funcs.m_set_cancel_next_tick_wait_callback = &cc_mqtt311_bm_client_set_cancel_next_tick_wait_callback;
    funcs.m_set_send_output_data_callback = &cc_mqtt311_bm_client_set_send_output_data_callback;
    funcs.m_set_broker_disconnect_report_callback = &cc_mqtt311_bm_client_set_broker_disconnect_report_callback;
    funcs.m_set_message_received_report_callback = &cc_mqtt311_bm_client_set_message_received_report_callback;
    funcs.m_set_error_log_callback = &cc_mqtt311_bm_client_set_error_log_callback;
    return funcs;
}