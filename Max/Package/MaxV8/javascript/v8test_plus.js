// set number of io and describe them
inlets = 2;
outlets = 1;

var leftval;
var rightval;

if(jsarguments.lenght > 1 && !isNaN(parseFloat(jsarguments[1])))
{
	rightval = jsarguments[1];
}

function bang()
{
	outlet(0, leftval + rightval);
}

function msg_float(value)
{
	if(inlet == 0)
	{
		leftval = value;
		bang();
	}
	else
	{
		rightval = value;
	}
}

function msg_int(value)
{
	if(inlet == 0)
	{
		leftval = value;
		bang();
	}
	else
	{
		rightval = value;
	}
}