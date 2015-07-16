// set number of io and describe them
inlets = 1;
outlets = 1;

var stringToEval = "";

function bang()
{
	post("eval : " + stringToEval);
	outlet(0, eval(stringToEval));
}

function set(v)
{
	var args = arrayfromargs(arguments)
	stringToEval = arguments.toString();
}