var url = require('url');
var http = require('http');
var express = require('express');
var app = express();

app.use(function (req, res, next) {
	next();
});

app.use(express.bodyParser());

app.get('/', function (req, res) {
	console.log('get called');
	res.writeHead(200, {
		'Content-Type': 'text/html',
		'Transfer-Encoding': 'chunked'
	})

	res.write("test");
	res.write("test");
	res.end();
});

app.use(function (err, req, res, next) {
	console.error(err.stack);
	res.send(500, 'Server error');
});

app.listen(8080);


