#include "Arduino.h"
#include <ArduinoJson.h>
#include "embeddedWoT_CoAP.h"

embeddedWoT_CoAP::embeddedWoT_CoAP(int port): port(port), ac_doc(2000), ia_doc(1000), ipia_doc(2000), e_doc(1000), es_doc(20), ipe_doc(2000), coap(udp) {    
    // events data
    ipe_arr = ipe_doc.createNestedArray("clients_list");
    ipia_arr = ipia_doc.createNestedArray("clients_list");

    this->coap.response([] (CoapPacket &packet, IPAddress ip, int port) {
        Serial.print("Coap Response got: ");
        
        char p[packet.payloadlen + 1];
        memcpy(p, packet.payload, packet.payloadlen);
        p[packet.payloadlen] = NULL;
        
        Serial.println(p);
    });
}

void embeddedWoT_CoAP::sendCoAPTXT(String txt, String event_endpoint) {
    for(i = 0; i < ipe_arr.size(); i++) {
        String ws_ip = ipe_arr[i]["ip"];
        int messageid = ipe_arr[i]["num"];
        JsonArray ae = e_doc[ws_ip];
        Serial.printf("Sending CoAP string %s. Triggered event %s. Sending to ip %s\n", txt.c_str(), event_endpoint.c_str(), ws_ip.c_str());
        for(j = 0; j < ae.size(); j++) {
            if(!ae[j][event_endpoint.c_str()].isNull() && ae[j][event_endpoint.c_str()]) {
                // unsigned char ws_num = ipe_doc[ws_ip];
                IPAddress ip;
                ip.fromString(ws_ip);
                coap.sendResponse(ip, this->port, messageid, txt.c_str());
                Serial.println("Done");
            }
        }
    }
}

void embeddedWoT_CoAP::loop() {
    this->coap.loop();
}

void embeddedWoT_CoAP::start() {
    this->coap.start();
}

void embeddedWoT_CoAP::test() {
    Serial.printf("Nel test %s\n", this->events_endpoint[0].c_str());
}

bool embeddedWoT_CoAP::_setEventHandled(String ip_s, int num) {
    bool done = false; 
    bool conn = false;
    JsonObject obj_e;
    JsonObject obj_ipe;
    IPAddress ip;
    ip.fromString(ip_s);
    
    if(e_doc[ip_s].isNull()) {
        Serial.println("isNull()");
        obj_e = e_doc.createNestedArray(ip_s).createNestedObject();
        obj_ipe = ipe_arr.createNestedObject();
        obj_ipe["ip"] = ip_s;
        obj_ipe["num"] = num;
        ipe_doc[ip_s] = num;
    }
    else {
        Serial.println("!isNull()");
        for(j = 0; !conn && j<e_doc[ip_s].size(); j++) {
            if(!e_doc[ip_s][j][events_endpoint[this->i]].isNull())
                conn = true;    
        }
        if(!conn)
            obj_e = e_doc[ip_s].createNestedObject();  
    }
    
    Serial.printf("conn status is %s\n", conn ? "true": "false");
    if(conn) {
        done = true;
        Serial.println("Provo ad inviare il pack");
        coap.sendResponse(ip, this->port, num, "Connection already established");
        Serial.println("Pack inviato");
    }
    else {
        done = true;
        Serial.println("Provo ad inviare il pack conconf");
        coap.sendResponse(ip, this->port, num, "Connection confirmed - event accomplished");
        Serial.printf("Pack inviato, hasbs: %s\n", this->hasSubscriptionSchema ? "true" : "false");
        if(this->hasSubscriptionSchema && this->events_subscriptionSchema[i]) 
            obj_e[events_endpoint[this->i]] = false;
        else {
            Serial.printf("Quindi finisco qua, i: %d\n", this->i);
            obj_e[events_endpoint[this->i]] = true;  
            Serial.println("Provo ad inviare il pack subconf");
            coap.sendResponse(ip, this->port, num, "Subscription confirmed");  
            Serial.println("Pack inviato");
        }
    }
    Serial.println("Funzione conclusa");
    return conn;
}
/*
bool embeddedWoT_CoAP::_setIAHandled(String ip_s, int num, String endpoint) {
    bool done = false; 
    bool conn = false;
    JsonObject obj_ia;
    JsonObject obj_ipia;
    //PREAMBOLO PER LA CONNESSIONE DI UN NUOVO HOST
    //Se é giá connesso lo avviso e faccio in modo che non venga fatto altro,
    //manipolando il fatto che si é ripresentato di nuovo lo stesso host di prima.
    if(ia_doc[ip_s].isNull()) {
        obj_ia = ia_doc.createNestedArray(ip_s).createNestedObject();
        obj_ia[endpoint] = true;
        obj_ipia = ipia_arr.createNestedObject();
        obj_ipia["ip"] = ip_s;
        obj_ipia["num"] = num;
        ipia_doc[ip_s] = num;
    }
    else {
        for(j = 0; !conn && j<ia_doc[ip_s].size(); j++) {
            if(!ia_doc[ip_s][j][endpoint].isNull())
                conn = true;
        }
        if(!conn)
            obj_ia = ia_doc[ip_s].createNestedObject();  
    }
    
    if(conn) {
        done = true;
        CoAP.sendTXT(num, "Connection already established");
    }
    return done;
}*/

void embeddedWoT_CoAP::bindEventSchema(DynamicJsonDocument doc) {
    int eventsBound = doc.size();
    JsonObject obj = doc.as<JsonObject>();
    //JsonObject::iterator it = obj.begin();
    int i = 0;
    //setter
    this->es_doc = doc;
    //for debug
    // serializeJsonPretty(this->es_doc, Serial);
    if (eventsBound == 0) {
        this->hasSubscriptionSchema = false;
    } else {
        this->hasSubscriptionSchema = true;
    }
    this->events_subscriptionSchema[eventsBound];
    this->events_cancellationSchema[eventsBound];

    for (JsonPair kw : obj) {
        // String key = it->key;
        Serial.printf("CoAP - Subscription for #%d is ", i);
        if (!doc[kw.key()]["subscription"].isNull()) {
            Serial.println("true");
            this->events_subscriptionSchema[i] = true;
        } else {
            Serial.println("false");
            this->events_subscriptionSchema[i] = false;
        }

        Serial.printf("CoAP - Cancellation for #%d is ", i);
        if (!doc[kw.key()]["cancellation"].isNull()) {
            Serial.println("true");
            this->events_subscriptionSchema[i] = true;
        } else {
            Serial.println("false");
            this->events_subscriptionSchema[i] = false;
        }
        i++;
    }
}

void embeddedWoT_CoAP::exposeProperties(const String *endpoints, properties_handler callbacks[], int prop_num) {
    // this->properties_endpoint = endpoints;
    // this->properties_cb = callbacks;

    // this->properties_number = prop_num;
    int i = 0;
    for(i = 0; i < prop_num; i++) {
        const char* endpoint = endpoints[i].c_str();
        //delete the first slash from the endpoint
        if (endpoint[0] == '/') {
            endpoint++;
        }
        //Generate callback 
        this->coap.server([this, callbacks, i] (CoapPacket &packet, IPAddress ip, int port) {
            String resp = callbacks[i]();
            Serial.printf("Responding to coap %s port %d\n", resp.c_str(), port);
            this->coap.sendResponse(ip, port, packet.messageid, resp.c_str());
        }, endpoint);
    }
}

void embeddedWoT_CoAP::exposeActions(const String *endpoints, actions_handler callbacks[], int act_num) {
    // this->actions_endpoint = endpoints;
    // this->actions_cb = callbacks;

    // this->actions_number = act_num;

    int i = 0;
    for(i = 0; i < act_num; i++) {
        const char* endpoint = endpoints[i].c_str();
        //delete the first slash from the endpoint
        if (endpoint[0] == '/') {
            endpoint++;
        }

        this->coap.server([this, callbacks, i] (CoapPacket &packet, IPAddress ip, int port) {
            // decode response
            char p[packet.payloadlen + 1];
            memcpy(p, packet.payload, packet.payloadlen);
            p[packet.payloadlen] = NULL;
            
            String message(p);

            String resp = callbacks[i](message);
            this->coap.sendResponse(ip, port, packet.messageid, resp.c_str());
        }, endpoint);
    }
}

void embeddedWoT_CoAP::exposeEvents(const String *endpoints, int evt_num) {
    this->events_endpoint = endpoints;

    this->events_number = evt_num;
    //Serial.printf("Events endpoint 0 is %s\n", this->events_endpoint[0].c_str());

    for(this->i = 0; this->i < evt_num; this->i++) {
        const char* endpoint = endpoints[i].c_str();
        //delete the first slash from the endpoint
        if (endpoint[0] == '/') {
            endpoint++;
        }

        this->coap.server([this] (CoapPacket &packet, IPAddress ip, int port) {
            // decode response
            char p[packet.payloadlen + 1];
            memcpy(p, packet.payload, packet.payloadlen);
            p[packet.payloadlen] = NULL;
            Serial.println("Fino qui non dovrebbero esserci problemi");
            
            this->_setEventHandled(ip.toString(), packet.messageid);
            // String message(p);

            // String resp = callbacks[i](message);
            // this->coap.sendResponse(ip, port, packet.messageid, resp.c_str());
        }, endpoint);
    }
}