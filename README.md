## DRAFT

# What is [nodecpp](http://bit.ly/Y3qP1R)?

This project is an attempt to manifest learnings from node.js, Ruby on Rails,
Python, and other frameworks into a new framework targeting C++11.

*TODO*: Discuss the pros and cons related to "coding by convention"
experienced by Ruby on Rails folks, and PHP folks, etc...

## Idea
Nodecpp is a cross-platform event-based application model for C++11. The
platform's fundamental design component is asynchronicity through continuations
via C++11 lambdas. For example:

	/* setup a server to dynamically return GET requests from an alternate host */
	http_get_route("/", [](const http_get_request_t &req, http_respond_t &respond) {
		http_get("http://example.com/proxy/dest", 80 /*port*/, [=](const http_response_t &res) {
			respond.send(res.fields["Content-type"], res.body);
		});
	});

The deliverable of this project will be a set of portable source code for use
for free as a base for new application development.  Along with this base, we
should be able to provide a set of components to accomplish basic tasks, like
accepting HTTP connections, connecting to file-shares, or starting multi-cast
IP transmissions. Longer term, we might aspire to building a responsive and
cancellable client-side graphical UI or rendering framework.

Threading and compute thoughts are around using thread pools for task dispatch,
but maintaining the main thread event loop for handling continuation after compute.

This project will rely on contributors like you. If you'd like to be involved,
please [get in touch](http://bit.ly/wbbradley-github).

*TODO*: discuss promises, futures, etc...

*TODO*: discuss how a framework in C++11 can equally (or better) address the
goals of the the ["coding by
convention"](http://en.wikipedia.org/wiki/Convention_over_configuration) folks.

*TODO*: discuss the bar for success when it comes to "ease of development."
Meaning, is there a way to know when/if we've created a platform that is more
efficient in terms of developer time than other application development
platforms?

*TODO*: discuss feelings of texty people coming from Ruby/Python around string
manipulation and whether C++11 syntax is or is not deep enough to address text
manipulation ease-of-use issues.

*TODO*: look into hooks for easy calls to/from other languages.

*TODO*: target non-goals, like having a [REPL](http://en.wikipedia.org/wiki/Read%E2%80%93eval%E2%80%93print_loop).

## Phase 1 - Proof of concept
In order to prove that this idea can scale, the first project should enable
serving static files from a front-end server at roughly the same benchmark
performance characterstics as high-speed servers such as nginx, and node.js.
(*TODO*: identify goal benchmarks.)

### HTTP Client
* Issuing requests
* Redirection handling
* Simple APIs
	* GET
	* POST

### HTTP Server
* Listening
* Parsing Request Headers
* Routing
	* Dispatch via the routes table to the appropriate app-defined callback
	* Enable middleware / Next() continuations
* http\_respond\_t design/impl
	* Simple Content-length
	* Chunking
	* Redirects
	* Errors API
* Keep-alive for multi-requests

### SSL integration (https)

### File I/O
* Basic read
* Basic write
* Open memory-mapped
* File enumeration
* Directory/File change notification
* Create directory
* Create file
* stat, etc...
* Flush

### JSON
* DOM
* Streaming

### Timers

### Express-like features
* Static file serving


## Later Phases
### Database connect
* RDBMS
	* MySQL
	* Postgres
* NoSQL
	* MongoDB
	* Redis
	* ...
* ORM

### IPC
* Pipes
* Shared memory
* Spawning processes

### Clustering

### Compute
* Compute tasks
* Task pool
* GPU compute integration

### WebSocket Server
* Upgrading from HTTP connections
* Investigate proper callback API design

### HTML parsing
* streaming templates

### MVC Framework

### XML
* DOM
* Streaming


### ...

# Build Instructions

* Install [md2pdf](https://github.com/joequery/md2pdf)

From within the nodepp directory:

1. `make -C deps/libuv`
1. `make -C deps/http_parser`
1. `make`

--
[Will Bradley](http://github.com/wbbradley)
