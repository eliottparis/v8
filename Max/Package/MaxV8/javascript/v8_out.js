// set number of io and describe them
inlets = 1;
outlets = 1;

function bang()
{
	outlet(0, "bang");
}

function msg_float(value)
{
	outlet(0, value);
}

function msg_int(value)
{
	outlet(0, value);
}

function out_arguments()
{
	var args = arrayfromargs(arguments);
	args.sort();
	outlet(0, args);
}

function out_array()
{
	var array = new Array();
	array[0] = 1.4;
	array[1] = 42;
	array[2] = "test";
	array[3] = "jojo zaza";
	outlet(0, array);
}

function out_arrayofarray()
{
	var firstArray = new Array();
	var secondArray = new Array();
	firstArray[0] = 1.4;
	firstArray[1] = 42;
	firstArray[2] = "test";
	secondArray[0] = firstArray;
	secondArray[1] = 23;

	outlet(0, secondArray);
}

function out_list()
{
	outlet(0, "toasty", 0.8, 42, "toto", "jojo zaza");
}

function out_regexp()
{
	var re = /(\w+)\s(\w+)/;
	var chaine = 'Alain Dupont';
	var nouvelleChaine = chaine.replace(re, '$2, $1');
	outlet(0, nouvelleChaine);
}

function out_undefined()
{
	var undefined_value;
	outlet(0, undefined_value);
}

function out_null()
{
	var null_value = null;
	outlet(0, null_value);
}