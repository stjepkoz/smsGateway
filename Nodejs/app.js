#!/usr/bin/node
const WebSocket = require('ws');
const express = require('express');
const uuid = require('uuid');
const fs = require('fs').promises;
const bp = require('body-parser');

const app = express();
app.use(bp.json())
app.use(bp.urlencoded({ extended: true }))

const wss = new WebSocket.Server({ port: 8080 });

let indexFile;

// Create a queue class
class Queue {
  // Constructor function
  constructor() {
    // Initialize an empty array for the queue
    this.queue = Array();
  }

  // Enqueue a new item
  enqueue(item) {
    // Add the item to the end of the queue
    this.queue.push(item);
  }

  // Dequeue an item
  dequeue() {
    // Return and remove the first item from the queue
    return this.queue.shift();
  }

  // Check if the queue is empty
  isEmpty() {
    // Return true if the queue is empty, false otherwise
    return this.queue.length === 0;
  }
  
  length() {
  	return this.queue.length;
  }
}

class Task {
	constructor(number, message) {
    	this.number = number;
    	this.message = message
    	this.uuid = uuid.v4();
    	//this.code = (Math.floor(Math.random() * 900000) + 100000).toString();
  }
}

class Client {
	constructor(ws) {
		this.uuid = uuid.v4();
		this.queue = new Queue();
		this.ws = ws
		this.currentTask;
		this.isAlive = true;
	}
	
	workload() {
		return this.queue.length();
	}
	
	addTask(task) {
		this.queue.enqueue(task);
		console.log(`Client ${this.uuid} added new task`);
		console.log(this.queue);
	}
	
	dequeueTask() {
		return this.queue.dequeue();
	}
	
	executeTask() {
		if (this.currentTask || this.workload() == 0) {
			return;
		}
		this.currentTask = this.dequeueTask();
		var object = this.currentTask;
		object.uuid = this.uuid;
		this.ws.send(JSON.stringify(this.currentTask));
		console.log(`Client ${this.uuid} executed ${this.currentTask.number} ${this.currentTask.message}`);
	}
	
	release() {
		this.currentTask = null;
	}
}

class ClientManager {
	constructor() {
		this.clients = new Map();
		this.generalQueue = new Queue();
	}
	
	hasClients() {
		return this.clients.size > 0;
	}
	
	addClient(client) {
		if (!this.hasClients() && !this.generalQueue.isEmpty()) {
			do { // when first client connects, transfer all tasks from general queue to client's queue
  				const task = this.generalQueue.dequeue();
  				client.addTask(task);
			} while (!this.generalQueue.isEmpty());
		}
		this.clients.set(client.uuid, client);
		console.log(`Client ${client.uuid} connected`);
	}
	
	addTaskToFreeClient(task) {
		if (!this.hasClients()) { // if there are no clients connected, add task to general queue
			console.log("No clients connected, add task to general queue");
			this.generalQueue.enqueue(task);
		} else {
			var minTasks = Number.POSITIVE_INFINITY;
			var selectedClient;
			for (let client of this.clients.values()){ //add new task to the client with the lowest number of the tasks in the queue
				console.log(`Client ${client.uuid} has ${client.workload()} tasks in queue`);
				const clientWorkload = client.workload();
				if (clientWorkload <= minTasks) {
					minTasks = clientWorkload;
					selectedClient = client;
				}
			}
			selectedClient.addTask(task);
		}
	}
	
	removeClient(client) {
		if (client) {
			this.clients.delete(client.uuid);
			if (client.currentTask) { //get tasks from client and divide them to other clients
				delete client.currentTask.uuid;
				this.addTaskToFreeClient(client.currentTask);
			}
			while (client.workload() > 0) {
  				this.addTaskToFreeClient(client.dequeueTask());
			} 
		}
	}
	
	execute() {
		for (let client of this.clients.values()){
			client.executeTask();
		}
	}
	
	relaseClientWithUUID(uuid) {
		const client = this.clients.get(uuid);
		if (client) {
			client.release();
		} 
	}
	
}

const clientManager = new ClientManager();

// Creating connection using websocket
wss.on("connection", ws => {
    const client = new Client(ws);
    clientManager.addClient(client);
    // sending message
    ws.on("message", data => {
        console.log(`Client has sent us: ${data}`);
        try {
        	const resultObj = JSON.parse(data);
        	if (resultObj.result == 1) {
        		clientManager.relaseClientWithUUID(resultObj.uuid);	
        	}
        }
        catch (e) {
  			console.log(e);
		}
    });
    // handling what to do when clients disconnects from server
    ws.on("close", data => {
    	console.log(`The client has disconnected: ${data}`);
        //clientManager.removeClient();
    });
    // handling client connection error
    ws.onerror = function () {
        console.log("Some Error occurred")
    }
    
    ws.on("pong", function () {
    	for (let client of clientManager.clients.values()){
    		if (client.ws === ws) {
    			client.isAlive = true;
    			break;
    		}
    	}
    });
});

setInterval(() => {
	if (!clientManager.hasClients()) {
		return;
	}
	clientManager.execute();
}, 1000),

setInterval(() => {
  for (let client of clientManager.clients.values()) { //periodically ping clients, and if they don't respond with pong, delete them
  	if (client.isAlive === false) { 
      	clientManager.removeClient(client);
      	client.ws.terminate();
      	console.log(`The client has disconnected: ${client.uuid}`);
      	return;
    }
    client.isAlive = false;
    client.ws.ping();
  }
}, 5000);


// Define an endpoint that returns a list of users
app.post('/api/gateway', (req, res) => {
  const number = req.body.number;
  const message = req.body.message;

  // Return an error if the "name" parameter is missing
  if (!number) {
    res.status(400).send({ error: 'Missing "number" parameter' });
    return;
  }
  if (!message) {
    res.status(400).send({ error: 'Missing "message" parameter' });
    return;
  }
  clientManager.addTaskToFreeClient(new Task(number, message));
  
  res.send({success: `Number ${number} and ${message} received` });
});

app.get('/sms/send', (req, res) => {
	res.setHeader("Content-Type", "text/html");
    res.writeHead(200);
    res.end(indexFile);
  
});

fs.readFile(__dirname + "/index.html")
    .then(contents => {
        indexFile = contents;
        app.listen(3000, () => {
            console.log('API Server listening on port 3000');
            console.log("The WebSocket server is running on port 8080");
        });
    })
    .catch(err => {
        console.error(`Could not read index.html file: ${err}`);
        process.exit(1);
    });
