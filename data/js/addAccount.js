/***************************************************************************
 * addAccount.js : UI control implementation for add account form
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

defaultPorts = {
    imap: {
	none: 143,
	ssl: 993,
	tls: 143
    },
    pop: {
	none: 110,
	ssl: 995,
	tls: 110
    },
    smtp: {
	none: 25,
	ssl: 465,
	tls: 25
    }
}

preconfiguredAccounts = {
    'gmail.com': {
	name: 'Gmail',
	incomingProtocol: 'imap',
	incomingServer: 'imap.gmail.com',
	incomingSecurity: 'ssl',
	incomingUseFullAddress: true,
	outgoingServer: 'smtp.gmail.com',
	outgoingSecurity: 'ssl',
	outgoingAuth: 'password',
	outgoingUseFullAddress: true,
    },
    'aim.com': {
	name: 'Gmail',
	incomingProtocol: 'imap',
	incomingServer: 'imap.aim.com',
	incomingSecurity: 'none',
	incomingUseFullAddress: false,
	outgoingServer: 'smtp.aim.com',
	outgoingSecurity: 'none',
	outgoingAuth: 'password',
	outgoingUseFullAddress: false,
    },
    'hotmail.com': {
	name: 'Hotmail',
	incomingProtocol: 'pop',
	incomingServer: 'pop3.live.com',
	incomingSecurity: 'ssl',
	incomingUseFullAddress: true,
	outgoingServer: 'smtp.live.com',
	outgoingSecurity: 'ssl',
	outgoingAuth: 'password',
	outgoingUseFullAddress: true,
    },
}

function updatePorts ()
{
    var incomingProtocol = $("input[name='incoming-protocol-choice']:checked").val();
    var incomingSecurity = $("input[name='incoming-security-choice']:checked").val();
    var outgoingSecurity = $("input[name='outgoing-security-choice']:checked").val();

    $("input[name='incoming-server-port']").val(defaultPorts[incomingProtocol][incomingSecurity]);
    $("input[name='outgoing-server-port']").val(defaultPorts['smtp'][outgoingSecurity]);
}

function fillFromAddress ()
{
    emailAddress = $("input[name='emailaddress']").val();
    parts = emailAddress.split('@', 2);

    if (parts.length < 2)
	return;

    if (parts[1] in preconfiguredAccounts) {
	settings = preconfiguredAccounts[parts[1]];
	$("input[type='radio'][name='incoming-protocol-choice']").attr("checked", false).checkboxradio('refresh');
	$("input[type='radio'][name='incoming-protocol-choice'][value='"+settings.incomingProtocol+"']").attr("checked", true).checkboxradio('refresh');
	if ('incomingSecurity' in settings) {
	    $("input[name='incoming-security-choice']").attr("checked", false).checkboxradio('refresh');
	    $("input[name='incoming-security-choice'][value='"+settings.incomingSecurity+"']").attr("checked", true).checkboxradio('refresh');
	}
	if ('outgoingSecurity' in settings) {
	    $("input[name='outgoing-security-choice']").attr("checked", false).checkboxradio('refresh');
	    $("input[name='outgoing-security-choice'][value='"+settings.outgoingSecurity+"']").attr("checked", true).checkboxradio('refresh');
	}
	if ('outgoingAuth' in settings) {
	    $("input[name='outgoing-auth-choice']").attr("checked", false).checkboxradio('refresh');
	    $("input[name='outgoing-auth-choice'][value='"+settings.outgoingAuth+"']").attr("checked", true).checkboxradio('refresh');
	}
	updatePorts();
	if ('incomingServer' in settings)
	    $("input[name='incoming-server-host']").val(settings.incomingServer);
	if ('incomingPort' in settings)
	    $("input[name='incoming-server-port']").val(settings.incomingPort);
	if ('outgoingServer' in settings)
	    $("input[name='outgoing-server-host']").val(settings.outgoingServer);
	if ('outgoingPort' in settings)
	    $("input[name='outgoing-server-port']").val(settings.outgoingPort);

	if (settings.incomingUseFullAddress)
	    $("input[name='incoming-server-username']").val(emailAddress);
	else
	    $("input[name='incoming-server-username']").val(parts[0]);
	if (settings.outgoingUseFullAddress)
	    $("input[name='outgoing-server-username']").val(emailAddress);
	else
	    $("input[name='outgoing-server-username']").val(parts[0]);
	if (settings.name && !$("input[name='accountname']").val())
	    $("input[name='accountname']").val(settings.name);
    } else {
	$("input[name='incoming-server-username']").val(parts[0]);
	$("input[name='outgoing-server-username']").val(parts[0]);
    }
}

function addAccount (data)
{
    iwkRequest ("addAccount", "Adding new account", {
	formData: data
    }).done(function (msg) {
	if (msg.is_ok) {
	    $.mobile.changePage("#page-accounts");
	    refreshAccounts();
	} else {
	    showError (msg.error);
	}
    });
}

$(function () {
    $("#form-add-account").submit(function () {
	try {
	    addAccount($(this).serialize());
	} catch (e) {
	    console.log(e.message);
	}
	return false;
    });

    $("#add-account-button").click(function () {
	clearForm($('#form-add-account'));
	return true;
    });

    $("#cancel-add-account").click(function () {
	$.mobile.changePage("#page-accounts");
    });

    $("input[name='incoming-security-choice']").change(function () {
	updatePorts();
    });

    $("input[name='outgoing-security-choice']").change(function () {
	updatePorts();
    });

    $("input[name='incoming-protocol-choice']").change(function () {
	updatePorts();
    });

    $("input[name='emailaddress']").change(function () {
	fillFromAddress ();
    });

    $("input[name='incoming-server-password']").change(function () {
	if (!$("input[name='outgoing-server-password']").val())
	    $("input[name='outgoing-server-password']").val($(this).val());
    });

    $("input[name='outgoing-server-password']").change(function () {
	if (!$("input[name='incoming-server-password']").val())
	    $("input[name='incoming-server-password']").val($(this).val());
    });

    updatePorts();

});
