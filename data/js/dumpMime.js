/***************************************************************************
 * dumpMime.js : dumps a MIME structure as obtained from iwk:getMessage
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

function mimeTypeIs (part, type, subtype)
{
    if (!('type' in part.mimeType))
	return false;

    if (part.mimeType.type == null || type != part.mimeType.type.toLowerCase())
	return false;

    if (subtype == "*")
	return true;

    if (!('subType' in part.mimeType))
	return false;

    return part.mimeType.subType != null && subtype == part.mimeType.subType.toLowerCase();

}

function isAttachment (part)
{
    if ('disposition' in part) {
	if (part.disposition != null && part.disposition.toLowerCase() == 'attachment') {
	    return true;
	}
    }
    return false;
}

function dumpBestAlternative (multipart, parent)
{
    best = -1;
    for (i in multipart.parts) {
	if (mimeTypeIs (multipart.parts[i], "text", "html")) {
	    best = i;
	    break;
	} else if (mimeTypeIs (multipart.parts[i], "text", "plain")) {
		best = i;
	}
    }

    if (best != -1)
	dumpDataWrapper (multipart.parts[i], parent);
}

function dumpMultipart (multipart, parent)
{
    if (mimeTypeIs (multipart, "multipart", "alternative")) {
	dumpBestAlternative (multipart, parent);
    } else if (mimeTypeIs (multipart, "multipart", "related")) {
	if (multipart.parts.length > 0)
	    dumpDataWrapper (multipart.parts[0], parent);
    } else {
	for (i in multipart.parts)
	    dumpDataWrapper (multipart.parts[i], parent);
    }
}

function dumpDataWrapper (dataWrapper, parent)
{
    if (dataWrapper.isMultipart) {
	dumpMultipart (dataWrapper, parent);
    } else if (dataWrapper.isMedium) {
	if (dataWrapper.isMessage) {
	    // Is message, see content (should also add headers)
	}
	if (dataWrapper.content.isMultipart) {
	    dumpDataWrapper (dataWrapper.content, parent);
	} else {
	    if ((mimeTypeIs (dataWrapper.content, "text", "html") ||
		 mimeTypeIs (dataWrapper.content, "text", "plain")) &&
		!isAttachment (dataWrapper)) {
		iframe = document.createElement ("iframe");
		iframe.setAttribute ("id", dataWrapper.content.uri);
		iframe.className += "iwk-part-iframe";
		iframe.setAttribute ("border", "0");
		iframe.setAttribute ("frameborder", "0");
		iframe.setAttribute ("src", dataWrapper.content.uri);
		iframe.setAttribute ("width", "100%");
		$(parent).append(iframe);
	    } else {
		div = document.createElement ("div");
		div.setAttribute("data-role", "controlgroup");
		div.setAttribute("data-type", "horizontal");
		div.setAttribute("data-mini", "true");
		// p = document.createElement ("p");
		// $(p).text("Attachment "+dataWrapper.filename);
		if (dataWrapper.filename)
		    $(div).text("Attachment "+dataWrapper.filename);
		else
		    $(div).text("Attachment "+dataWrapper.contentType.type+"/"+dataWrapper.contentType.subType);		    
		open = document.createElement ("a");
		open.setAttribute("data-role", "button");
		open.setAttribute("data-inline", "true");
		open.setAttribute("href", dataWrapper.content.uri+"?mode=open");
		$(open).text("Open");
		$(open).button();
		save = document.createElement ("a");
		save.setAttribute("data-role", "button");
		save.setAttribute("data-inline", "true");
		save.uri=dataWrapper.content.uri;
		save.setAttribute("href", dataWrapper.content.uri+"?mode=save");
		$(save).text("Save");
		$(save).button();
		$(div).append(open);
		$(div).append(save);
		// $(div).append(p);
		$(parent).append(div);
		$("#page-message").trigger("create");
	    }
	}
    } else {
    }
}

function bestAlternativeGetBodies (multipart)
{
    result = [];
    best = -1;
    for (i in multipart.parts) {
	if (mimeTypeIs (multipart.parts[i], "text", "plain")) {
	    best = i;
	    break;
	} else if (mimeTypeIs (multipart.parts[i], "text", "html")) {
		best = i;
	}
    }

    if (best != -1)
	result = dataWrapperGetBodies (multipart.parts[i]);

    return result;

    return [];
}

function multipartGetBodies (multipart)
{
    result = []
    if (mimeTypeIs (multipart, "multipart", "alternative")) {
	result =  bestAlternativeGetBodies (multipart);
    } else if (mimeTypeIs (multipart, "multipart", "related")) {
	if (multipart.parts.length > 0)
	    result = dataWrapperGetBodies (multipart.parts[0]);
    } else {
	for (i in multipart.parts)
	    if (!multipart.parts[i].isMessage)
		result = result.concat (dataWrapperGetBodies (multipart.parts[i]));
    }

    return result;
}

function dataWrapperGetBodies (dataWrapper)
{
    result = []
    if (dataWrapper.isMultipart) {
	result = multipartGetBodies (dataWrapper);
    } else if (dataWrapper.isMedium) {
	if (dataWrapper.content.isMultipart) {
	    result = dataWrapperGetBodies (dataWrapper.content);
	} else {
	    if ((mimeTypeIs (dataWrapper.content, "text", "html") ||
		 mimeTypeIs (dataWrapper.content, "text", "plain")) &&
		!isAttachment (dataWrapper)) {
		result = [dataWrapper.content];
	    }
	}
    }

    return result;
}

