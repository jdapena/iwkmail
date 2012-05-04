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

function dumpAccountInAccountsList (account, parent)
{
    li = document.createElement("li");
    li.setAttribute('id', 'account-item-'+account.id);
    li.setAttribute('data-role', 'fieldcontain');
    a = document.createElement("a");
    a.setAttribute('href', '#page-folders?account='+account.id);
    a.accountId = account.id;
    $(a).click(function () {
	globalStatus.currentAccount = this.accountId;
	globalStatus.currentFolder = null;
	globalStatus.currentMessage = null;
	fillFoldersList(this.accountId);
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
	a.accountId=accountId;
	a.folderFullName=folderFullName;
	a.displayName = folder.displayName;
	a.setAttribute('href', '#page-messages');
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
    li = document.createElement("li");
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