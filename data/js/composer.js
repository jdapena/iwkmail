/***************************************************************************
 * composer.js : Composer UI control implementation for main-view.html
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

function fillComposerFrom (accounts)
{
    $("#composer-from-choice").html("");
    for (i in accounts) {
	dumpAccountOptionInComposerFrom (accounts[i], "#composer-from-choice");
    }
}

function clearComposer ()
{
    clearForm($('#form-composer'));
    $("#composer-attachments-list").empty();

    $("#composer-body").empty();

    legend = document.createElement("legend");
    $(legend).text("Attachments:");

    addAttachment = document.createElement("a");
    addAttachment.setAttribute('href', '#');
    addAttachment.setAttribute('data-role', 'button');
    addAttachment.setAttribute('data-icon', 'plus');
    addAttachment.setAttribute('id', 'add-attachment-button');
    $(addAttachment).text("Add attachment");

    $(addAttachment).click(function () {
	attachFiles();
    });

    $("#composer-attachments-list").append(legend);
    $("#composer-attachments-list").append(addAttachment);

    composerSetDirty (false);
}

function forward ()
{
    clearComposer ();
    message = globalStatus.messageStructure;
    $.mobile.changePage("#composer");
    if (message.subject.indexOf("Fw:") != 0 &&
	message.subject.indexOf("FW:") != 0 &&
	message.subject.indexOf("Fwd:") != 0 &&
	message.subject.indexOf("FWD:") != 0) {
	newSubject = "Fwd: "+message.subject;
    } else {
	newSubject = message.subject;
    }
    $("#composer-subject").val(newSubject);
    $("#composer-to").val("");
    $("#composer-cc").val("");

    bodies = dataWrapperGetBodies (message);

    // We should somehow concatenate the bodies. Now we simply use the first body we find.
    if (bodies.length > 0) {
	$.ajax({
	    url: bodies[0].uri,
	    dataType: 'jsonp',
	}).done(function(result) {
	    body = "\n-- Original message --\n"+result.data;
	    $("#composer-body").text(body);
	});
    }
}

function reply (who)
{
    clearComposer();
    message = globalStatus.messageStructure;
    $.mobile.changePage("#composer");
    if (message.subject.indexOf("Re:") != 0 &&
	message.subject.indexOf("RE:") != 0) {
	newSubject = "Re: "+message.subject;
    } else {
	newSubject = message.subject;
    }
    $("#composer-subject").val(newSubject);
    to = "";
    if (who == 'mailingList') {
	to = message.mlist;
    } else {
	for (i in message.from) {
	    if (to.length > 0)
		to += ", ";
	    if (message.from[i].displayName.length > 0) {
		to += ('"'+message.from[i].displayName + '" ');
		to += "<"+message.from[i].emailAddress +">";
	    } else {
		to += message.from[i].emailAddress;
	    }
	}
    }
    cc = "";
    if (who == 'all') {
	for (i in message.cc) {
	    if (cc.length > 0)
		cc += ", ";
	    if (message.cc[i].displayName.length > 0) {
		cc += ('"'+message.cc[i].displayName + '" ');
		cc += "<"+message.cc[i].emailAddress +">";
	    } else {
		cc += message.cc[i].emailAddress;
	    }
	}
	for (i in message.to) {
	    if (cc.length > 0)
		cc += ", ";
	    if (message.to[i].displayName.length > 0) {
		cc += ('"'+message.to[i].displayName + '" ');
		cc += "<"+message.to[i].emailAddress +">";
	    } else {
		cc += message.to[i].emailAddress;
	    }
	}
    }
    $("#composer-to").val(to);
    $("#composer-cc").val(cc);

    bodies = dataWrapperGetBodies (message);

    // We should somehow concatenate the bodies. Now we simply use the first body we find.
    if (bodies.length > 0) {
	$.ajax({
	    url: bodies[0].uri,
	    dataType: 'jsonp',
	}).done(function(result) {
	    body = "\n-- Original message --\n"+result.data;
	    $("#composer-body").text(body);
	});
    }
}

function replySender ()
{
    reply('sender');
}

function replyAll ()
{
    reply('all');
}

function replyMailingList ()
{
    reply('mailingList');
}

function askReply ()
{
    message = globalStatus.messageStructure;
    if (message.mlist && message.mlist.length > 0) {
	$("#reply-mailing-list-button").show();
    } else {
	$("#reply-mailing-list-button").hide();
    }
    $.mobile.changePage("#reply-to-dialog");
}

function attachFiles ()
{
    iwkRequest ("openFileURI", "Choosing attachment to add", {
	title: 'Add attachments...',
	attachAction: 'Attach'
    }).done(function (msg) {
	if (msg.uris) {
	    for (i in msg.uris) {
		item = document.createElement ("a");
		item.className += "iwk-attachment-item";
		item.isAttachment = true;
		item.uri = msg.uris[i];
		item.setAttribute('href', '#');
		item.setAttribute('data-role', 'button');
		item.setAttribute('data-icon', 'minus');
		$(item).text(uriGetFilename (msg.uris[i]));

		$(item).click(function () {
		    $(this).remove();
		    composerChanged();
		});
		$("#composer-attachments-list").append(item);
		composerSetDirty(true);
	    }
	}
	$("#composer-attachments-list").trigger("create");
    });
}

function composerSend (data)
{
    iwkRequest ("composerSend", "Adding to outbox", {
	formData: data
    }).done(function (msg) {
	if (!msg.is_ok)
	    showError (msg.error);
	else
	    syncFolders();
    });
}

var _composerDirty = false;

function composerSetDirty (dirty)
{
    _composerDirty = dirty;
}

function composerIsDirty ()
{
    return _composerDirty;
}

function saveDraft ()
{
    iwkRequest ("composerSaveDraft", "Saving to drafts", {
	formData: data
    }).done(function (msg) {
	if (!msg.is_ok)
	    showError (msg.error);
	else
	    syncFolders ();
    });
}

function discardChanges ()
{
    /* One for closing dialog, another for closing composer */
    history.go (-2);
}

$(function () {
    $(".iwk-compose-button").click(function () {
	clearComposer();
	return true;
    });

    $("#composer-send").click(function () {
	attachments = [];
	$(".iwk-attachment-item").each(function () {
	    attachments[attachments.length] = this.uri;
	});
	$("#composer-attachments").val(attachments.join(","));
	$("#form-composer").submit();
    });

    $("#composer-back").click(function () {
	if (composerIsDirty ()) {
	    $.mobile.changePage("#save-draft-or-discard-dialog")
	    return false;
	} else {
	    return true;
	}
    });

    $("#form-composer").submit(function () {
	composerSend($(this).serialize());
	return false;
    });

    $("#form-composer :input").change(function () {
	composerSetDirty(true);
    });

});
