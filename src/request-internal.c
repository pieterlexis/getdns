/**
 *
 * /brief getdns contect management functions
 *
 * This is the meat of the API
 * Originally taken from the getdns API description pseudo implementation.
 *
 */
/* The MIT License (MIT)
 * Copyright (c) 2013 Verisign, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "types-internal.h"
#include "util-internal.h"

/* useful macros */
#define gd_malloc(sz) context->memory_allocator(sz)
#define gd_free(ptr) context->memory_deallocator(ptr)

void network_req_free(getdns_network_req* net_req) {
    if (!net_req) {
        return;
    }
    getdns_context_t context = net_req->context;
    if (net_req->pkt) {
        ldns_pkt_free(net_req->pkt);
    }
    gd_free(net_req);
}

getdns_network_req* network_req_new(getdns_context_t context,
                                    const char* name,
                                    uint16_t request_type,
                                    struct getdns_dict* extensions) {
    getdns_network_req *net_req = NULL;
    ldns_pkt *pkt = NULL;
    net_req = gd_malloc(sizeof(getdns_network_req));
    if (!net_req) {
        return NULL;
    }
    net_req->ns = NULL;
    net_req->pkt = NULL;
    net_req->context = context;
    net_req->request_type = request_type;
    
    /* create ldns packet */
    pkt = create_new_pkt(context, name, request_type, extensions);
    if (!pkt) {
        /* free up the req */
        network_req_free(net_req);
        return NULL;
    }
    net_req->pkt = pkt;
    
    return net_req;
}

void dns_req_free(getdns_dns_req* req) {
    if (!req) {
        return;
    }
    getdns_context_t context = req->context;
    network_req_free(req->current_req);
    gd_free(req);
}

/* create a new dns req to be submitted */
getdns_dns_req* dns_req_new(getdns_context_t context,
                                   const char* name,
                                   uint16_t request_type,
                                   struct getdns_dict *extensions,
                                   getdns_transaction_t *transaction_id) {
    getdns_dns_req *result = NULL;
    getdns_network_req *net_req = NULL;
    result = gd_malloc(sizeof(getdns_dns_req));
    if (result == NULL) {
        return NULL;
    }
    result->context = context;
    result->current_req = NULL;
    result->pending_cb = 0;
    result->resolver_type = context->resolution_type;
    
    /* create the initial network request */
    net_req = network_req_new(context, name, request_type,
                              extensions);
    if (!net_req) {
        dns_req_free(result);
        result = NULL;
    }
    
    result->trans_id = ldns_pkt_id(net_req->pkt);
    if (transaction_id) {
        *transaction_id = result->trans_id;
    }

    result->current_req = net_req;
    net_req->owner = result;
    
    return result;
}