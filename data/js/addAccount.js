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

function updatePorts ()
{
    var incomingProtocol = $("input[name='incoming-protocol-choice']:checked").val();
    var incomingSecurity = $("input[name='incoming-security-choice']:checked").val();
    var outgoingSecurity = $("input[name='outgoing-security-choice']:checked").val();

    $("input[name='incoming-server-port']").val(defaultPorts[incomingProtocol][incomingSecurity]);
    $("input[name='outgoing-server-port']").val(defaultPorts['smtp'][outgoingSecurity]);
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

    $("input[name='incoming-security-choice']").change(function () {
	updatePorts();
    });

    $("input[name='outgoing-security-choice']").change(function () {
	updatePorts();
    });

    $("input[name='incoming-protocol-choice']").change(function () {
	updatePorts();
    });

    updatePorts();

});
