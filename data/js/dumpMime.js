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
	    console.log("Is message, see content (should also add headers)");
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
		p = document.createElement ("p");
		$(p).text("Attachment "+dataWrapper.filename);
		$(parent).append(p);
	    }
	}
    } else {
    }
}

