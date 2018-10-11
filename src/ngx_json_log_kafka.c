/*
 * Copyright (C) 2017 Paulo Pacheco
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <ngx_core.h>

#if (NGX_HAVE_LIBRDKAFKA)

#include <ngx_json_log_kafka.h>
#include <ngx_json_log_str.h>

#define NGX_JSON_LOG_KAFKA_ERROR_MSG_LEN (2048)

/* kafka configuration */
rd_kafka_topic_conf_t *
ngx_json_log_kafka_topic_conf_new(ngx_pool_t *pool) {
    rd_kafka_topic_conf_t *topic_conf = rd_kafka_topic_conf_new();
    if (!topic_conf) {
        ngx_log_error(NGX_LOG_CRIT, pool->log, 0,
                "json_log: Error allocating kafka topic conf");
    }
    return topic_conf;
}


/* create new configuration */
rd_kafka_conf_t *
ngx_json_log_kafka_conf_new(ngx_pool_t *pool)
{
    rd_kafka_conf_t *conf = rd_kafka_conf_new();
    if (!conf) {
        ngx_log_error(NGX_LOG_CRIT, pool->log, 0,
                "json_log: Error allocating kafka conf");
    }
    return conf;
}


/* set an integer config value */
ngx_int_t
ngx_json_log_kafka_conf_set_int(ngx_pool_t *pool,
    rd_kafka_conf_t *conf, const char * key, intmax_t value)
{
    char buf[NGX_INT64_LEN + 1];
    char errstr[NGX_JSON_LOG_KAFKA_ERROR_MSG_LEN];
    uint32_t errstr_sz = sizeof(errstr);
    rd_kafka_conf_res_t ret;

    snprintf(buf, NGX_INT64_LEN, "%lu", value);
    ret = rd_kafka_conf_set(conf, key, buf, errstr, errstr_sz);
    if (ret != RD_KAFKA_CONF_OK) {
        ngx_log_error(NGX_LOG_WARN, pool->log, 0,
                "json_log: failed to set kafka conf [%s] => [%s] : %s",
                key, buf, errstr);
        return NGX_ERROR;
    }
    return NGX_OK;
}


/* set a string config value */
ngx_int_t
ngx_json_log_kafka_conf_set_str(ngx_pool_t *pool, rd_kafka_conf_t *conf,
    const char * key, ngx_str_t *str)
{
    char errstr[NGX_JSON_LOG_KAFKA_ERROR_MSG_LEN];
    uint32_t errstr_sz = sizeof(errstr);
    rd_kafka_conf_res_t ret;

    u_char *value = ngx_json_log_str_dup(pool, str);
    if (!value) {
        ngx_log_error(NGX_LOG_WARN, pool->log, 0,
                "json_log: failed to set kafka conf [%s] : %s",
                key, errstr);
        return NGX_ERROR;
    }

    ret = rd_kafka_conf_set(conf, key, (const char *) value, errstr, errstr_sz);
    if(ret != RD_KAFKA_CONF_OK) {
        ngx_log_error(NGX_LOG_WARN, pool->log, 0,
                "json_log: failed to set kafka conf [%s] => [%s] : %s",
                key,(const char *) value, errstr);
        return NGX_ERROR;
    }
    return NGX_OK;
}


ngx_int_t
ngx_json_log_kafka_topic_conf_set_str(ngx_pool_t *pool,
    rd_kafka_topic_conf_t *topic_conf, const char *key, ngx_str_t *str)
{
    char errstr[NGX_JSON_LOG_KAFKA_ERROR_MSG_LEN];
    uint32_t errstr_sz = sizeof(errstr);
    rd_kafka_conf_res_t ret;

    u_char *value = ngx_json_log_str_dup(pool, str);
    if (!value) {
        ngx_log_error(NGX_LOG_WARN, pool->log, 0,
                "json_log: failed to set kafka topic conf [%s] : %s",
                key, errstr);
        return NGX_ERROR;
    }

    ret = rd_kafka_topic_conf_set(topic_conf,
            key, (const char *) value, errstr, errstr_sz);
    if(ret != RD_KAFKA_CONF_OK) {
        ngx_log_error(NGX_LOG_WARN, pool->log, 0,
                "json_log: failed to set kafka topic conf [%s] => [%s] : %s",
                key,(const char *) value, errstr);
        return NGX_ERROR;
    }
    return NGX_OK;
}


/* create a kafka handler for producing messages */
rd_kafka_t *
ngx_json_log_kafka_producer_new(ngx_pool_t *pool, rd_kafka_conf_t *conf)
{
    rd_kafka_t *rk                    = NULL;
    char errstr[NGX_JSON_LOG_KAFKA_ERROR_MSG_LEN];
    uint32_t errstr_sz = sizeof(errstr);

    /* Create Kafka handle */
    if (!(rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, errstr_sz))) {
        ngx_log_error(NGX_LOG_CRIT, pool->log, 0,
                "json_log: error allocating kafka handler");
        return NULL;
    }

    return rk;
}


size_t
ngx_json_log_kafka_add_brokers(ngx_pool_t *pool,
    rd_kafka_t *rk, ngx_array_t *brokers)
{
    ngx_str_t    *rec;
    ngx_str_t    *broker;
    u_char       *value = NULL;
    size_t       ret = 0;

    rec = brokers->elts;
    size_t i;
    for (i = 0; i < brokers->nelts; ++i) {

        broker = &rec[i];
        value = ngx_json_log_str_dup(pool, broker);

        if ( rd_kafka_brokers_add(rk, (const char *) value)){
            ngx_log_error(NGX_LOG_INFO, pool->log, 0,
                    "json_log: broker \"%V\" configured", broker);
            ++ret;
        } else {
            ngx_log_error(NGX_LOG_WARN, pool->log, 0,
                    "json_log: failed to configure \"%V\"", broker);
        }
    }
    return ret;
}


/* creates a configured topic */
rd_kafka_topic_t *
ngx_json_log_kafka_topic_new(ngx_pool_t *pool,
    rd_kafka_t *rk, rd_kafka_topic_conf_t *topic_conf, ngx_str_t *topic)
{
    u_char *value = ngx_json_log_str_dup(pool, topic);
    rd_kafka_topic_t * rkt;
    if (! rk ) {
        ngx_log_error(NGX_LOG_CRIT, pool->log, 0,
                "json_log: missing kafka handler");
        return NULL;
    }

    rkt = rd_kafka_topic_new(rk, (const char *)value, topic_conf);
    if(!rkt) {
        /* FIX ME - Why sooooo quiet! */
        ngx_log_error(NGX_LOG_WARN, pool->log, 0,
                "json_log: failed to create topic \"%V\"", topic);
    }

    return rkt;
}


ngx_int_t
ngx_json_log_init_kafka(ngx_pool_t *pool,
        ngx_json_log_main_kafka_conf_t *kafka)
{
    if (! kafka) {
        return NGX_ERROR;
    }

    kafka->rk                  = NULL;
    kafka->rkc                 = NULL;

    kafka->brokers             = ngx_array_create(pool, 1, sizeof(ngx_str_t));

    if (! kafka->brokers) {
        return NGX_ERROR;
    }

    kafka->client_id.data      = NULL;
    kafka->compression.data    = NULL;
    kafka->log_level           = NGX_CONF_UNSET_UINT;
    kafka->max_retries         = NGX_CONF_UNSET_UINT;
    kafka->buffer_max_messages = NGX_CONF_UNSET_UINT;
    kafka->backoff_ms          = NGX_CONF_UNSET_UINT;
    kafka->partition           = NGX_CONF_UNSET;

    return NGX_OK;
}


ngx_int_t
ngx_json_log_configure_kafka(ngx_pool_t *pool,
        ngx_json_log_main_kafka_conf_t *conf)
{
    /* kafka */
    /* configuration kafka constants */
    static const char *conf_client_id_key          = "client.id";
    static const char *conf_compression_codec_key  = "compression.codec";
    static const char *conf_log_level_key          = "log_level";
    static const char *conf_max_retries_key        = "message.send.max.retries";
    static const char *conf_buffer_max_msgs_key    = "queue.buffering.max.messages";
    static const char *conf_retry_backoff_ms_key   = "retry.backoff.ms";

    /* - default values - */
    static ngx_str_t  kafka_compression_default_value = ngx_string("snappy");
    static ngx_str_t  kafka_client_id_default_value = ngx_string("nginx");
    static ngx_int_t  kafka_log_level_default_value = 6;
    static ngx_int_t  kafka_max_retries_default_value = 0;
    static ngx_int_t  kafka_buffer_max_messages_default_value = 100000;
    static ngx_msec_t kafka_backoff_ms_default_value = 10;

#if (NGX_DEBUG)
    static const char *conf_debug_key              = "debug";
    static ngx_str_t   conf_all_value              = ngx_string("all");
#endif

    /* create kafka configuration */
    conf->rkc = ngx_json_log_kafka_conf_new(pool);
    if (! conf->rkc) {
        return NGX_ERROR;
    }

    /* configure compression */
    if ((void*) conf->compression.data == NULL) {
        conf->compression = kafka_compression_default_value;
        ngx_json_log_kafka_conf_set_str(pool, conf->rkc,
                conf_compression_codec_key,
                &kafka_compression_default_value);
    } else {
        ngx_json_log_kafka_conf_set_str(pool, conf->rkc,
                conf_compression_codec_key,
                &conf->compression);
    }
    /* configure max messages, max retries, retry backoff default values if unset*/
    if (conf->buffer_max_messages == NGX_CONF_UNSET_UINT) {
        ngx_json_log_kafka_conf_set_int(pool, conf->rkc,
                conf_buffer_max_msgs_key,
                kafka_buffer_max_messages_default_value);
    } else {
        ngx_json_log_kafka_conf_set_int(pool, conf->rkc,
                conf_buffer_max_msgs_key,
                conf->buffer_max_messages);
    }
    if (conf->max_retries == NGX_CONF_UNSET_UINT) {
        ngx_json_log_kafka_conf_set_int(pool, conf->rkc,
                conf_max_retries_key,
                kafka_max_retries_default_value);
    } else {
        ngx_json_log_kafka_conf_set_int(pool, conf->rkc,
                conf_max_retries_key,
                conf->max_retries);
    }
    if (conf->backoff_ms == NGX_CONF_UNSET_MSEC) {
        ngx_json_log_kafka_conf_set_int(pool, conf->rkc,
                conf_retry_backoff_ms_key,
                kafka_backoff_ms_default_value);
    } else {
        ngx_json_log_kafka_conf_set_int(pool, conf->rkc,
                conf_retry_backoff_ms_key,
                conf->backoff_ms);
    }
    /* configure default client id if not set*/
    if ((void*) conf->client_id.data == NULL) {
        ngx_json_log_kafka_conf_set_str(pool, conf->rkc,
                conf_client_id_key,
                &kafka_client_id_default_value);
    } else {
        ngx_json_log_kafka_conf_set_str(pool, conf->rkc,
                conf_client_id_key,
                &conf->client_id);
    }
    /* configure default log level if not set*/
    if (conf->log_level == NGX_CONF_UNSET_UINT) {
        ngx_json_log_kafka_conf_set_int(pool, conf->rkc,
                conf_log_level_key,
                kafka_log_level_default_value);
    } else {
        ngx_json_log_kafka_conf_set_int(pool, conf->rkc,
                conf_log_level_key,
                conf->log_level);
    }

    /* configure partition */
    if (conf->partition == NGX_CONF_UNSET) {
        conf->partition = RD_KAFKA_PARTITION_UA;
    }

#if (NGX_DEBUG)
    /* configure debug */
    ngx_json_log_kafka_conf_set_str(pool,conf->rkc,
            conf_debug_key,
            &conf_all_value);
#endif
    /* create kafka handler */
    conf->rk = ngx_json_log_kafka_producer_new(
            pool,
            conf->rkc);

    if (! conf->rk) {
        return NGX_ERROR;
    }
    /* set client log level */
    if (conf->log_level == NGX_CONF_UNSET_UINT) {
        rd_kafka_set_log_level(conf->rk,
                kafka_log_level_default_value);
    } else {
        rd_kafka_set_log_level(conf->rk,
                conf->log_level);
    }
    /* configure brokers */
    conf->valid_brokers = ngx_json_log_kafka_add_brokers(pool,
            conf->rk, conf->brokers);

    if (!conf->valid_brokers) {
        ngx_log_error(NGX_LOG_ALERT, pool->log, 0,
                "json_log: failed to configure at least a kafka broker.");
        return NGX_ERROR;
    }

    return NGX_OK;
}


void
ngx_json_log_kafka_topic_disable_ack(ngx_pool_t *pool,
    rd_kafka_topic_conf_t *rktc)
{
    static const char *conf_req_required_acks_key  = "request.required.acks";
    static ngx_str_t   conf_zero_value             = ngx_string("0");

    if (! pool) {
        return;
    }

    if (! rktc) {
        return;
    }

    ngx_json_log_kafka_topic_conf_set_str(pool, rktc,
            conf_req_required_acks_key, &conf_zero_value);
}

void
ngx_json_log_kafka_produce(ngx_pool_t *pool,
    rd_kafka_t *rk,
    rd_kafka_topic_t * rkt,
    ngx_int_t partition,
    char * txt, ngx_str_t *msg_id)
{
    int                             err;

    /* FIXME : Reconnect support */
    /* Send/Produce message. */
    if ((err = rd_kafka_produce(
                    rkt,
                    partition,
                    RD_KAFKA_MSG_F_COPY,
                    /* Payload and length */
                    txt, strlen(txt),
                    /* Optional key and its length */
                    msg_id && msg_id->data ? (const char *) msg_id->data: NULL,
                    msg_id ? msg_id->len : 0,
                    /* Message opaque, provided in
                     * delivery report callback as
                     * msg_opaque. */
                    NULL)) == -1) {

        ngx_log_error(NGX_LOG_ERR, pool->log, 0,
                "%% Failed to produce to topic %s "
                "partition %i: %s - %d\n",
                rd_kafka_topic_name(rkt),
                partition,
#if RD_KAFKA_VERSION < 0x000b01ff
               rd_kafka_err2str(rd_kafka_errno2err(err)),
#else
               rd_kafka_err2str(rd_kafka_last_error()),
#endif
               err);
    }
#if (NGX_DEBUG)

    if (err) {
        ngx_log_error(NGX_LOG_DEBUG, pool->log, 0,
                "http_json_log: kafka msg:[%s] ERR:[%d] QUEUE:[%d]",
                txt, err, rd_kafka_outq_len(rk));
    }
#endif

}

#endif// (NGX_HAVE_LIBRDKAFKA)
