
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