#include <stdio.h>
#include <stdlib.h>

#include <mosquitto.h>

// thie function is called after client is connected to protocol, that is, mosquitto_connect_callback_set();
// (client's object, pointer to client's id, return code(error code))
void on_connect(struct mosquitto *mosq, void *obj, int rc){
	printf("ID: %d\n", *(int *) obj);

	// if connection was failed, rc would be greater 0.
	if(rc){
		printf("Error with result code: %d\n", rc);
		exit(-1);
	}
	
	// now connection is successful!
	
	// register client as subscriber
	// (client's obj, msg id, topic, QoS)
	mosquitto_subscribe(mosq, NULL, "test/hong/cpu", 0);
}

// this function will be called whenvever a message is arrived from broker
// (client's object, msg id pointer, msg data)
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg){
	printf("New message with topic %s: %s\n", msg->topic, (char *) msg->payload);
}

int main(){
	int rc, id = 12;

	// initialize library
	mosquitto_lib_init(); 

	struct mosquitto *mosq;
	
	// create client's instance. (client's name, flag for clean session, pointer of client's id)
	mosq = mosquitto_new("subscribe-test", true, &id); 

	// set callback functions.
	// on_connect is called after client is connected to broker
	// on_message is called after client received message
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_message_callback_set(mosq, on_message);

	// Now try to connect to broker
	// (client's obj, hostname of our broker, port number, timeout(sec))
	rc = mosquitto_connect(mosq, "broker.hivemq.com", 1883, 10);
	if(rc){
		printf("Could not connect to Broker with return code %d\n", rc);
		return -1;
	}

	mosquitto_loop_start(mosq);
	printf("Press Enter to quit…\n");
	getchar();
	mosquitto_loop_stop(mosq, true);

	// disconnect broker
	mosquitto_disconnect(mosq);
	// destroy client's obj
	mosquitto_destroy(mosq);
	// clean up library
	mosquitto_lib_cleanup();

	return 0;
}
