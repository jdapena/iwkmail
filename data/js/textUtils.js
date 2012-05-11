/***************************************************************************
 * textUtils.js : Helpers for strings formatting, including email addresses
 ***************************************************************************/

/*
 * Authors:
 *  Jose Dapena Paz <jdapena@igalia.com>
 *
 * Copyright (c) 2012, Igalia, S.L.
 * All rights reserved.
 *
 * See license.js
 */

function trim(s)
{
    return s.replace(/^\s*|\s*$/g,"");
}

function trimQuotes(s) {
    return s.replace(/^[\s\"]*|[\s\"]*$/g,"");
}

function splitAddressesList (str)
{
    afterAt = false;
    len = str.length;

    if (len == 0)
	return [];
    start = 0;


    while (start < len && 
	   (" ,;\n".match(str[start])))
	start++;

    if (start == len)
	return [];

    end = start + 1;
    while (end < len && str[end] != ';' && !(afterAt && str[0] == ',')) {
	if (str[end] == '"') {
	    Seed.print("Found quote at end "+end);
	    while (end < len && str[end] != '"')
		end++;
	}
	if (str[end] == '@') {
	    afterAt = true;
	}
	if ((end<len && str[end] == '>')&&((end + 1 < len) && str[end+1] == ',')) {
	    end++;
	    break;
	}
	end++;
    }

    return [str.substring(start, end)].concat(splitAddressesList(str.substring(end+1)));
}

function addressGetDisplay(address)
{
    inQuotes = false;
    if (address == null || address.length == 0)
	return "(no recipient)";

    for (i = 0; i < address.length; i++) {
	if (inQuotes) {
	    if (address[i] == '"')
		inQuotes = false;
	} else if (address[i] == '"') {
	    inQuotes = true;
	} else if (address[i] == '<') {
	    return trimQuotes(address.substring (0, i-1));
	}
    }

    return trimQuotes(address);
}

function addressesGetDisplay(str)
{
    if (str == null)
	return "(no recipient)";
    recipients = splitAddressesList (str);
    result = "";
    for (i in recipients) {
	recipients[i] = addressGetDisplay(recipients[i]);
    }
    return recipients.join(", ");
}

function uriGetFilename (uri)
{
    var a = document.createElement("a");
    a.href = uri;

    return a.pathname.split('/').pop();
}

