// set number of io and describe them
inlets = 1;
var nouts = 0;
var currentOut = 0;

function bang()
{
	currentOut = Math.round(currentOut % nouts);
	outlet(currentOut++, "bang");
}

var init = function()
{
	function isNumeric(num)
	{
    	return !isNaN(num)
	}

	outlets = (jsarguments.length > 1 && isNumeric(jsarguments[1])) ? jsarguments[1] : 5;
	nouts = outlets;
}

init();