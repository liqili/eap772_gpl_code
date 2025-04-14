// JavaScript Document
(function($){

$.su.Proxy = function(options){
	var defaults = {
		xtype: "proxy",
		url: null,
		timeout: 16000,
		reader: {
			url: null,
			root: "data",
			type: null,
			timeout: null
		},
		writer: {
			url: null,
			root: "data",
			type: null,
			timeout: null
		},
		type: "POST",
		dataType: "json"
	};

	var settings = $.extend(defaults, options);
	
	//reader
	settings.reader.url = settings.reader.url || settings.url;
	settings.reader.type = settings.reader.type || settings.type;
	settings.reader.timeout = settings.reader.timeout || settings.timeout;
	settings.reader.type = settings.reader.type.toUpperCase();
	//writer
	settings.writer.url = settings.writer.url || settings.url;
	settings.writer.type = settings.writer.type || settings.type;
	settings.writer.timeout = settings.writer.timeout || settings.timeout;
	settings.writer.type = settings.writer.type.toUpperCase();
	

	$.extend(this, settings);
	this.isProxy = true;
};

$.su.Proxy.prototype.read = function(data, callback, callback_fail, callback_error, triggerEvent, scope){
	var data = $.extend({
		"operation": "read"
	}, data);
	
	var me = scope || this,
		callback = callback || function(){},
		callback_fail = callback_fail || function(){},
		callback_error = callback_error || function(){},
		triggerEvent = triggerEvent === false ? false: true;

	var url = me.reader.url;
	return $.ajax({
		url: me.reader.url,
		type: me.reader.type,
		timeout: me.reader.timeout,
		dataType: me.dataType,
		cache: false,
		data: data,
		traditional: true,
		success: function(data, status, xhr){
			if (data.success){
				if (true == data.timeout)
				{
					window.top.location.href = "/";
					return
				}
			
				var root = me.reader.root,
					result = null;
				if (root){
					result = data[root];
					//console.log(url);
				}else{
					result = data;
					//console.log(url);
				};
				//console.log("proxy", data);
				callback.call(me, result, data.others, status, xhr);
				if (triggerEvent){
					$(me).trigger("ev_read", [result, data.others, status, xhr]);
				};
			}else{
				//进入配置失败机制
				if (data.errorcode === "timeout"){
					//alert("Timeout Error!");
					location.href = "/";
				}else{
					callback_fail.call(me, data.errorcode, data.others, data.data);
					$(me).trigger("ev_failed", [data.errorcode, data.others, data.data]);
				};
			};
		},
		error: function(xhr, status, type){
			//console.log(url);
			//console.log("proxy", arguments)
			if (!me.preventErrorDefault){
				//console.log("error", xhr, status, type, url);
				//alert("Data Error!");
			};
			callback_error.call(me, xhr, status, type, url);

		}
	});
};

$.su.Proxy.prototype.write = function(data, callback, callback_fail, callback_error, triggerEvent, scope){
	var data = $.extend({
		"operation": "write"
	}, data);
	
	var me = scope || this,
		callback = callback || function(){},
		callback_fail = callback_fail || function(){},
		callback_error = callback_error || function(){},
		triggerEvent = triggerEvent === false ? false: true;

	return $.ajax({
		url: me.writer.url,
		type: me.writer.type,
		timeout: me.writer.timeout,
		dataType: me.dataType,
		cache: false,
		data: data,
		traditional: true,
		success: function(data, status, xhr){
			if (data.success){
				if (true == data.timeout)
				{
					window.top.location.href = "/";
				}
				
				var root = me.writer.root,
					result = null;
				if (root){
					result = data[root];
				}else{
					result = data;
				}
				callback.call(me, result, data.others, status, xhr);
				if (triggerEvent){
					$(me).trigger("ev_write", [result, data.others, status, xhr]);
				};
			}else{
				//进入配置失败机制
				if (data.errorcode === "timeout"){
					//alert("Timeout Error!");
					location.href = "/";
				}else{
					//console.log(callback_fail)
					callback_fail.call(me, data.errorcode, data.others, data.data);
					$(me).trigger("ev_failed", [data.errorcode, data.others, data.data]);
				};
			}
		},
		error: function(xhr, status, type){
			if (!me.preventErrorDefault){
				//console.log("error", xhr, status, type);
				//alert("Data Error!");
			};
			callback_error.call(me, xhr, status, type);
		}
	});
};


})(jQuery);
