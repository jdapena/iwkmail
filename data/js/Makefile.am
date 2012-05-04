IWK_JS=iwk.js

IWK_JS_FILES=\
	license.js \
	config.js \
	dateUtils.js \
	htmlUtils.js \
	dumpMime.js \
	dumpItems.js \
	textUtils.js \
	common.js \
	mainView.js \
	addAccount.js

$(IWK_JS): $(IWK_JS_FILES)
	cat $(IWK_JS_FILES) > $(IWK_JS)

JS_FILES=\
	jquery-1.7.1.min.js \
	jquery.mobile-1.1.0.min.js \
	$(IWK_JS)

jsdir = $(datadir)/iwkmail/js
js_DATA= $(JS_FILES)

