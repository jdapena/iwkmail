/***************************************************************************
 * mainView.js : UI control implementation for main-view.html
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

function fillAccountsList (accounts)
{
    clearAccountsList ();

    for (i in accounts) {
	dumpAccountInAccountsList (accounts[i], "#page-accounts #accounts-list");
    }
    dumpAddAccountInAccountsList ("#page-accounts #accounts-list");

    $("#page-accounts #accounts-list").listview('refresh');
}

function fillAccountsListCounts ()
{
    $("#page-accounts #accounts-list").listview();

    for (i in globalStatus.folders) {
	account = globalStatus.folders[i];
	accountId = account.accountId;
	inbox = account.folders["INBOX"];
	if (!inbox)
	    inbox = account.folders["inbox"];
	accountItemId = "account-item-"+accountId;
	if (inbox && inbox.unreadCount && inbox.unreadCount > 0) {
	    $("#page-accounts #account-item-"+accountId+" .account-count").text(inbox.unreadCount);
	    $("#page-accounts #account-item-"+accountId+" .account-count").show();
	} else {
	    $("#page-accounts #account-item-"+accountId+" .account-count").hide();
	}
    }
    $("#page-accounts #accounts-list").listview('refresh');
}

function fillFoldersList(accountId)
{
    for (i in globalStatus.folders) {
	account = globalStatus.folders[i];
	if (account.accountId == accountId) {
	    $("#page-folders-title").text(account.accountName + " folders");

	    $("#page-folders #folders-list").html("");
	    for (fullName in account.folders) {

		folder = account.folders[fullName];
		dumpFolderInFolderList (accountId, fullName, folder,
					"#page-folders #folders-list");
		
		if (fullName == "INBOX") {
		    accountItemId = "account-item-"+accountId;
		    $("#page-accounts #"+accountItemId+" .countSpan:first").text(folder.unreadCount);
		    if (folder.unreadCount > 0)
			$("#page-accounts #"+accountItemId+" .countSpan:first").show();
		    else
			$("#page-accounts #"+accountItemId+" .countSpan:first").hide();
		}
	    }
	    $("#page-folders #folders-list").listview('refresh');

	    break;
	}
    }
}

function fillMessageViewHeader (message)
{
    dumpMessageAsViewHeader (message, "#page-message");
}

function fillMessageViewBody (message)
{
    globalStatus.messageStructure = message;
    dumpDataWrapper (message, "#page-message #iframe-container");
}

function clearMessageViewBody ()
{
    $("#page-message #iframe-container").text("");
}

function clearAccountsList ()
{
    $("#accounts-list").html("");
}

function abortShowMessages ()
{
    if ('showMessages' in globalStatus.requests) {
	globalStatus.requests["showMessages"].abort();
    }
}

function markMessageAsRead(uid)
{
    iwkRequest ("flagMessage", "Marking message as read", {
	account: globalStatus.currentAccount,
	folder: globalStatus.currentFolder,
	message: uid,
	setFlags: "seen"
    }).done(function (msg) {
	$("#message-item-"+uid).removeClass("iwk-unread-item");
	$("#message-item-"+uid).addClass("iwk-read-item");
	$("#message-view-mark-as-read").hide();
	$("#message-view-mark-as-unread").show()
    });
}

function markMessageAsUnread(uid)
{
    iwkRequest ("flagMessage", "Marking message as unread", {
	account: globalStatus.currentAccount,
	folder: globalStatus.currentFolder,
	message: uid,
	unsetFlags: "seen"
    }).done(function (msg) {
	$("#message-item-"+uid).removeClass("iwk-read-item");
	$("#message-item-"+uid).addClass("iwk-unread-item");
	$("#message-view-mark-as-unread").hide();
	$("#message-view-mark-as-read").show();
    });
}

function markAsRead ()
{
    markMessageAsRead (globalStatus.currentMessage);
}

function markAsUnread ()
{
    markMessageAsUnread (globalStatus.currentMessage);
}

function forward ()
{
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

function showMessage(message)
{
    globalStatus.messageStructure = null;
    clearMessageViewBody ();
    fillMessageViewHeader (message);
    if ('getMessage' in globalStatus.requests) {
	globalStatus.requests['getMessage'].abort();
    }

    globalStatus.requests["getMessage"] = iwkRequest ("getMessage", "Getting message", {
	account: globalStatus.currentAccount,
	folder: globalStatus.currentFolder,
	message: message.uid
    }).done(function (msg) {
	fillMessageViewBody (msg.result);
	markMessageAsRead (message.uid);
    }).always(function(jqXHR, textStatus, errorThrown) {
	if ('getMessage' in globalStatus.requests)
	    delete globalStatus.requests["getMessage"];
    });
}

function showNextMessage()
{
    current = null;
    $("#messages-list .iwk-message-item-link").each (function (index) {
	if (current != null) {
	    globalStatus.currentMessage = this.messageUid;
	    showMessage (this.message);
	    return false;
	}
	if (this.messageUid == globalStatus.currentMessage)
	    current = this;
    });
    $("#message-view-next-button").removeClass("ui-btn-active");
    return true;
}

function showPreviousMessage ()
{
    previous = null;
    $("#messages-list .iwk-message-item-link").each (function (index) {
	if (this.messageUid == globalStatus.currentMessage) {
	    if (previous) {
		globalStatus.currentMessage = previous.messageUid;
		showMessage (previous.message);
	    }
	    return false;
	}
	previous = this;
    });
    $("#message-view-previous-button").removeClass("ui-btn-active");
    return true;
}

function showMessages(accountId, folderId, onlyNew)
{
    try {
	if ('showMessages' in globalStatus.requests) {
	    globalStatus.requests["showMessages"].abort();
	}

	$("#messages-list-get-more-list").hide();
	$("#messages-list-getting-more-list").show();
	$("#messages-refresh").hide();
	$("#messages-cancel").show();

	retrieveCount = onlyNew?0:SHOW_MESSAGES_COUNT;
	globalStatus.requests["showMessages"] = iwkRequest ("getMessages", "Fetching messages", {
	    account: accountId,
	    folder: folderId,
	    newestUid: globalStatus.newestUid,
	    oldestUid: globalStatus.oldestUid,
	    count: retrieveCount
	}).done(function (msg) {
	    if (msg.newMessages.length > 0) {
		globalStatus.newestUid = msg.newMessages[0].uid;
	    }
	    if (globalStatus.newestUid == null && msg.messages.length > 0) {
		globalStatus.newestUid = msg.messages[0].uid;
	    }
	    if (msg.messages.length > 0) {
		globalStatus.oldestUid = msg.messages[msg.messages.length - 1].uid;
	    }
	    msg.newMessages.reverse();
	    for (i in msg.newMessages) {
		dumpMessageInMessagesList (msg.newMessages[i], true, "#page-messages #messages-list");
	    }
	    for (i in msg.messages) {
		dumpMessageInMessagesList (msg.messages[i], false, "#page-messages #messages-list");
	    }
	    $("#page-messages #messages-list").listview('refresh');
	}).always(function(jqXHR, textStatus, errorThrown) {
	    if ('showMessages' in globalStatus.requests)
		delete globalStatus.requests["showMessages"];
	    $("#messages-list-get-more-list").show();
	    $("#messages-list-getting-more-list").hide();
	    $("#messages-refresh").show();
	    $("#messages-cancel").hide();
	});
    } catch (e) {
	console.log(e.message);
    }
}

function fetchMoreMessages ()
{
    showMessages (globalStatus.currentAccount, globalStatus.currentFolder, false);
}

function fetchNewMessages ()
{
    showMessages (globalStatus.currentAccount, globalStatus.currentFolder, true);
}

function refreshAccounts ()
{
    iwkRequest ("getAccounts", "Updating accounts", {
    }).done(function (msg) {
	fillComposerFrom (msg.result);
	fillAccountsList (msg.result);
	syncFolders();
    });
}

function syncFolders ()
{
    iwkRequest("syncFolders", "Updating accounts and folders", {
    }).done(function (msg) {
	globalSetFolders (msg.result);
	fillAccountsListCounts ();
	fillFoldersList(globalStatus.currentAccount);
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

$(function () {
    $("#composer-send").click(function () {
	$("#form-composer").submit();
    });

    $("#form-composer").submit(function () {
	composerSend($(this).serialize());
	return false;
    });

    $("#accounts-compose").click(function () {
	clearForm($('#form-composer'));
	return true;
    });

    refreshAccounts();
});
