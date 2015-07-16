// set number of io and describe them
inlets = 1;
outlets = 8;

var months = [
"January", 
"February",
"March",
"April",
"May",
"June",
"July",
"August",
"September",
"October",
"November",
"December"];

function bang()
{
	var date = new Date();
	outlet(0, date.getDate());
	outlet(1, months[date.getMonth()]);
	outlet(2, date.getFullYear());
	outlet(3, date.getHours() + "h");
	outlet(4, date.getMinutes() + "min");
	outlet(5, date.getSeconds() + "sec");
	outlet(6, date.getMilliseconds());

	outlet(7, date.toDateString());
}