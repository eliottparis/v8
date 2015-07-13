// set number of io and describe them
inlets = 3;
outlets = 4;
setinletassist(0, "inlet assist test");
//setoutletassist(0, "outlet assist test");

var zozo = 42;
var fakeVersion = "1.4.5";
post("hello v8");
post("zozo = " + zozo);
post("fake version : " + fakeVersion);
error("this is an error !!!");

var zozo = function()
{
	outlets--;
	//post("posting in zozo function !");
}

zozo();

function loadbang()
{
	post("this is the 'loadbang' function :)", "jojo");
	post("The object has now " + outlets + " outlets");

	var toto = 3;
	for(var i = 0; i < 10; i++)
	{
		post("toto : " + toto++);
	}
}

function bang()
{
	//zozo = 32;
	//inlets = 8;
	post("bang");
	//post();
}