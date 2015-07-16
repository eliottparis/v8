// set number of io and describe them

inlets = 3;
outlets = 4;
setinletassist(0, "inlet assist test");
//setinletassist(2, "inlet assist test");
//setoutletassist(0, "outlet assist test");

post("Math.round(8.448) = " + Math.round(8.448));
post("Math.PI = " + Math.PI);

var nBangCalled = 0;
error("this is an error !!!");

function msg_float(f)
{
	post("(js) msg_float fn");
	post("(js) arg : " + f);
	post("(js) inlet " + inlet);
}

var zozo = function()
{
	post("posting in zozo function !");
	
	post("outlets was " + outlets +" and now is " + --outlets);
	post("outlets is now " + outlets);
}

zozo();

var testargs = function()
{
	post("(js) testargs fn");
	for (var i = 0; i < arguments.length; i++) {
		post("(js) args " + i + " : " + arguments[i]);
	};
}

function msg_int(v)
{
	post("(js) msg_int method");
	post("(js) arg : " + v);
}

function loadbang()
{
	post("this is the 'loadbang' function :)", "jojo");
	post("The object has now " + outlets + " outlets");
}

function bang()
{
	post("has " + jsarguments.length +" args")
	for(var i = 0; i < jsarguments.length; i++)
	{
		post("arg " + i + " : " + jsarguments[i]);
	}

	nBangCalled++;
	post("hey ! this is the bang function !!!");
	post("-- bang called " + nBangCalled + " times");
	outlet(0, 1.4, 55.8);
}

function testout()
{
	//outlet(0, arguments);
	outlet(0, arguments[0], arguments[1], arguments[2]);
	outlet(1, arguments[1], arguments[0], arguments[2]);
}