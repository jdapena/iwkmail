/***************************************************************************
 * dumpItems.js : Methods for dumping query results to lists and selectors
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

function getAccountInbox (accountId)
{
    for (i in globalStatus.folders) {
	account = globalStatus.folders[i];
	if (accountId == account.accountId) {
	    inbox = account.folders["INBOX"];
	    if (!inbox)
		inbox = account.folders["inbox"];
	    return inbox;
	}
    }

    return null;
}


function dumpAccountInAccountsList (account, parent)
{
    li = document.createElement("li");
    li.setAttribute('id', 'account-item-'+account.id);
    li.setAttribute('data-role', 'fieldcontain');
    a = document.createElement("a");
    a.setAttribute('href', '#page-messages');
    a.accountId = account.id;
    $(a).click(function () {
	globalStatus.currentMessage = null;
	fillFoldersList(this.accountId);
	newFolder = getAccountInbox (this.accountId);

	if (newFolder == null)
	    return false;

	if (globalStatus.currentAccount != this.accountId || globalStatus.currentFolder != folder.fullName) {
	    globalStatus.currentAccount = this.accountId;
	    globalStatus.currentFolder = newFolder.fullName;
	    globalStatus.currentmessage = null;
	    globalStatus.newestUid = null;
	    globalStatus.oldestUid = null;
	    $("#page-messages-title").text();
	    $("#page-message-title").text(folder.displayName);
	    $("#page-messages #messages-list").html("");
	    showMessages(this.accountId, newFolder.fullName, false);
	} else {
	    $("#messages-list-get-more-list").show();
	    $("#messages-list-getting-more-list").hide();
	}
	return true;
    });
	    
    h3 = document.createElement("h3");
    $(h3).text(account.displayName);
    p = document.createElement("p");
    if (account.isDefault)
	$(p).text(account.emailAddress + " (default)")
    else
	$(p).text(account.emailAddress);
    countSpan = document.createElement("span");
    countSpan.className += " ui-li-count account-count";
    $(countSpan).hide();
    $(countSpan).text(0);
    a.appendChild(h3);
    a.appendChild(p);
    a.appendChild(countSpan);
    li.appendChild(a);
    $(parent).append(li);	    
}

function dumpAddAccountInAccountsList (parent)
{
    li = document.createElement ("li");
    a = document.createElement ("a");
    a.setAttribute("href", "#create-account");
    a.setAttribute("id", "add-account-button");
    a.setAttribute("data-role", "button");
    a.setAttribute("data-icon", "plus");
    a.className += "account-item";
    a.innerText = "Add account";
    
    li.appendChild(a);
    $(parent).append(li);
}

function dumpAccountOptionInComposerFrom (account, parent)
{
    li = document.createElement("li");
    composerFromOption=document.createElement("option");
    composerFromOption.setAttribute("id", "composer-from-"+account.id);
    composerFromOption.setAttribute('value', account.id);
    composerFromOption.account = account.id;
    if (account.isDefault)
	composerFromOption.setAttribute("selected", "true");
    $(composerFromOption).text(account.emailAddress);
    $(parent).append(composerFromOption);
}


function dumpMessageAsViewHeader (message, parent)
{
    outgoing = message.draft || getCurrentFolder().isSent;
    if (outgoing)
	$(parent+" #recipient").text(addressesGetDisplay (message.to));
    else
	$(parent+" #recipient").text(addressGetDisplay (message.from));
    $(parent+" #subject").text(message.subject);
    $(parent+" #date").text(formatTime (message.dateReceived));
}

function dumpAddressDetail(display, email, parent)
{
    console.log("Dumping address [display:"+display+"] [email:"+email+"]");
    newLi = document.createElement('li');
    newLi.setAttribute('data-role', 'fieldcontain');
    newHeader = document.createElement('h3');
    if (display && display.length > 0) {
	$(newHeader).text(display);
	newP = document.createElement('p');
	$(newP).text(email);
	$(newLi).append(newHeader);
	$(newLi).append(newP);
    } else {
    	$(newHeader).text(email);
	$(newLi).append(newHeader);
    }
    $(parent).append(newLi);
}

function dumpDetail(fieldName, fieldValue, parent)
{
    newLi = document.createElement('li');
    newLi.setAttribute('data-role', 'fieldcontain');
    newP = document.createElement('p');
    $(newP).text(fieldName);
    newHeader = document.createElement('h3');
    $(newHeader).text(fieldValue);
    newLi.appendChild(newP);
    newLi.appendChild(newHeader);
    $(parent).append(newLi);
}

function dumpMessageAsDetails (message, parent)
{
    $(parent).empty();

    if (message.from && message.from.length > 0) {
    	$(parent).append("<li data-role='list-divider'>From:</li>");
    	for (i in message.from) {
	    dumpAddressDetail (message.from[i].displayName, message.from[i].emailAddress, parent);
    	}
    }
    if (message.to && message.to.length > 0) {
    	$(parent).append("<li data-role='list-divider'>To:</li>");
    	for (i in message.to) {
	    dumpAddressDetail (message.to[i].displayName, message.to[i].emailAddress, parent);
    	}
    }
    if (message.cc && message.cc.length > 0) {
    	$(parent).append("<li data-role='list-divider'>Cc:</li>");
    	for (i in message.cc) {
	    dumpAddressDetail (message.cc[i].displayName, message.cc[i].emailAddress, parent);
    	}
    }
    if (message.bcc && message.bcc.length > 0) {
    	$(parent).append("<li data-role='list-divider'>Bcc:</li>");
    	for (i in message.bcc) {
	    dumpAddressDetail (message.bcc[i].displayName, message.bcc[i].emailAddress, parent);
    	}
    }

    $(parent).append("<li data-role='list-divider'>Details</li>");
    dumpDetail ('Subject:', message.subject, parent);
    if (message.mlist)
	dumpDetail ('Mailing list:', message.mlist, parent);
    console.log($("#page-message-details").html());

    if ($(parent).hasClass("ui-listview")) {
	$(parent).listview("refresh");
    }
}

function dumpFolderInFolderList (accountId, folderFullName, folder, parent)
{
    li = document.createElement("li");
    li.setAttribute('id', 'folder-item-'+accountId+'-'+folderFullName);
    li.setAttribute('data-role', 'fieldcontain');

    h3 = document.createElement("h3");
    if ('fullDisplayName' in folder)
	$(h3).text(folder.fullDisplayName);
    else
	$(h3).text(folder.displayName);

    if (!folder.noSelect) {
	a = document.createElement("a");
	a.setAttribute("href", "#");
	a.setAttribute("data-rel", "back");
	a.accountId=accountId;
	a.folderFullName=folderFullName;
	a.displayName = folder.displayName;
	$(a).click(function () {
	    if (globalStatus.currentAccount != this.accountId || globalStatus.currentFolder != this.folderFullName) {
		globalStatus.currentAccount = this.accountId;
		globalStatus.currentFolder = this.folderFullName;
		globalStatus.currentmessage = null;
		globalStatus.newestUid = null;
		globalStatus.oldestUid = null;
		$("#page-messages-title").text(this.displayName);
		$("#page-message-title").text(this.displayName);
		$("#page-messages #messages-list").html("");
		showMessages(this.accountId, this.folderFullName, false);
	    } else {
		$("#messages-list-get-more-list").show();
		$("#messages-list-getting-more-list").hide();
	    }
	    return true;
	});
    }

    if ('unreadCount' in folder && folder.unreadCount > 0) {
	countSpan = document.createElement("span");
	countSpan.className += " ui-li-count";
	$(countSpan).text(folder.unreadCount);
	a.appendChild(countSpan);
	unreadCount = folder.unreadCount;
    }

    if (folder.noSelect) {
	li.appendChild(h3);
    } else {
	a.appendChild(h3);
	li.appendChild(a);
    }
    $(parent).append(li);
}

function dumpMessageInMessagesList (message, isNew, parent)
{
    if (message.deleted && !getCurrentFolder().isTrash)
	return;
    li = document.createElement("li");
    $(li).attr("id", "message-item-"+message.uid);
    $(li).attr("data-iwk-message-id", message.uid);
    li.className += " iwk-message-item";
    li.setAttribute('data-role', 'fieldcontain');
    if (message.unread)
	li.className += " iwk-unread-item";
    else
	li.className += " iwk-read-item";
    a = document.createElement("a");
    a.className += "iwk-message-item-link";
    a.setAttribute("href", "#page-message");
    a.accountId = globalStatus.currentAccount;
    a.folderFullName = globalStatus.currentFolder
    a.messageUid = message.uid;
    a.message = message;
    $(a).click(function () {
	globalStatus.currentAccount = this.accountId;
	globalStatus.currentFolder = this.folderFullName
	globalStatus.currentMessage = this.messageUid;
	showMessage(this.message);
	return true;
    });
    h3 = document.createElement("h3");
    $(h3).text(message.subject);
    p = document.createElement("p");
    outgoing = message.draft || getCurrentFolder().isSent;
    if (outgoing) {
	$(p).text(addressesGetDisplay(message.to));
    } else {
	$(p).text(addressGetDisplay(message.from));
    }
    date = document.createElement("p");
    date.className += "ui-li-aside";
    $(date).text(formatTime(message.dateReceived));
    a.appendChild(h3);
    a.appendChild(p);
    a.appendChild(date);
    li.appendChild(a);

    if (isNew)
	$(parent).prepend(li);
    else
	$(parent).append(li);
}