Anderson Chauphan
chaup001@umn.edu

# PROGRAM DESCRIPTION:

The following contents of this project directory contains a RFC/1945 HTTP/1.0 compliant webserver. The source code is located
in the `src` directory as a file named `server.c` and can be compiled to a binary to be executed. This binary functions as a
webserver that takes in HTTP/1.0 requests and responds back to the client with a HTTP/1.0 response containing a header and body
component. The HTTP header component contains necessary information such as HTTP status code, content-length, content-type, etc..
The HTTP body component will either contain the requested file from the server or, if the file does not exist, an error message.

This webserver is able to handle various file request types such as plain text, .html, .gif, .jpg, and .jpeg files. These types
of files will be able to be sent in the body of the server's HTTP response correctly. Other requested file types will have
undefined behavior and sent as a plain text type. 

Included in the project structure is a `conf` directory meant to hold server configuration files. The only configuration file
this webserver has been designed to read is from a file named `httpd.conf` which defined the following values the order:

- max amount of simultaneous connections
- path to content root directory
- path to default index.html file
- port number used by server

> By the default httpd.conf, the server will be accessable from the address `localhost:8888`. 

Additionally, this webserver contains a Common Gateway Interface (CGI) for GET and POST http request types. These types of request
will launch the associated GET and POST CGI binaries that are located in `cgi-bin`. The implemented CGI for both GET and POST requests
are written in Python 3.9.5 (see requirements section). Python CGI scripts will be able to dynamically process user inputs
from forms. Included in the project are two example forms for CGI requests, `content/form2.html` for GET request and
`content/form.html` for POST request. The GET request example simply sends the server the request file if it exists. The POST
request example outputs in plain text the two inputs given to it in the body of the HTTP response.

This webserver also contains a logging feature. Events that are logged are file accesses (to a file named `logs/access.log`) and errors
pertaining to the webserver program (to a file named `logs/error.log`). 

# REQUIREMENTS

A valid Python executable must be present on the server's machine.
The webserver will check for the following locations for a valid binary:

- `/usr/bin/python3`
- `/usr/bin/python`

As mentioned in the program description, Python version 3.9.5 was used when developing the CGI GET & POST scripts. It is recommended
that this version or higher of Python be made available in the mentioned file paths.
