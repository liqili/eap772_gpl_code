// JavaScript Document
(function($){


$.su.Widget("msg", {
	defaults: {
		type: "prompt", //alert, prompt, show, window
		cls: "xl",
		_title: "",
		autoshow: false,
		mask: true,
		msg: "",
		callback: null,
		okHandler: null,	//function
		noHandler: null,	//function
		cancelHandler: null,	//function
		closeBtn: true,
		global: false
	},
	create: function(defaults, options){
		var me = this;
		me.each(function(i, obj){

			if (this.isMsg){
				//已经初始化了，不再生成
				return;
			};

			var container = $(this);
			$.extend(this, defaults, options);
			if (this.global){
				if (!this.id){
					//console.error("You are init a global Tip, so you have to set an id for it!");
					return null;
				};
			};
			this.id = this.id || $.su.randomId("msg");
			
			var	inHTML =	'';
				

				inHTML += 	"<div class=\"msg-wrap\">";
				
			if (this.closeBtn){
				inHTML +=		"<a class=\"widget-close msg-close\" href=\"javascript:void(0);\"></a>";
			};

				inHTML +=		"<div class=\"msg-content-wrap\">";
			//if (this._title){
			var title = this._title || "";
				inHTML +=			"<h3 class=\"widget-title msg-title\">";
				inHTML +=				"<span class=\"msg-title-container\">"+title+"</span>";
				inHTML +=			"</h3>";
			//};
				
			var content = this.msg || this.innerHTML;
				container.empty();

				inHTML +=			"<div class=\"widget-content msg-content-container\">"+content+"</div>";
				inHTML +=		"</div>";				

			var type = this.type;
			switch (type){
				case "alert":
					inHTML +=	"<div class=\"msg-btn-container\">";
					inHTML +=		"<div class=\"msg-btn-wrap\">";
					inHTML +=			"<div class=\"button-container inline-block\">";
					inHTML +=				"<button type=\"button\" class=\"button-button btn-msg btn-msg-ok btn-alert\">"
					inHTML +=					"<span class=\"text button-text\">"+$.su.CHAR.OPERATION.OK+"</span>";
					inHTML +=				"</button>";
					inHTML +=			"</div>";
					inHTML +=		"</div>";
					inHTML +=	"</div>";

					break;
				case "confirm":
					inHTML +=	"<div class=\"msg-btn-container\">";
					inHTML +=		"<div class=\"msg-btn-wrap\">";
					inHTML +=			"<div class=\"button-container inline-block\">";
					inHTML +=				"<button type=\"button\" class=\"button-button btn-msg btn-msg-no btn-confirm\">"
					inHTML +=					"<span class=\"text button-text\">"+$.su.CHAR.OPERATION.NO+"</span>";
					inHTML +=				"</button>";
					inHTML +=			"</div>";
					inHTML +=			"<div class=\"button-container inline-block\">";
					inHTML +=				"<button type=\"button\" class=\"button-button btn-msg btn-msg-ok btn-confirm\">"
					inHTML +=					"<span class=\"text button-text\">"+$.su.CHAR.OPERATION.YES+"</span>";
					inHTML +=				"</button>";
					inHTML +=			"</div>";
					inHTML +=		"</div>";
					inHTML +=	"</div>";

					break;
				case "prompt":
					inHTML +=	"<div class=\"button-container msg-btn-container\">";
					inHTML +=		"<div class=\"msg-btn-wrap\">";
					inHTML +=			"<div class=\"button-container inline-block\">";
					inHTML +=				"<button type=\"button\" class=\"button-button btn-msg btn-msg-cancel btn-prompt\">"
					inHTML +=					"<span class=\"text button-text\">"+$.su.CHAR.OPERATION.CANCEL+"</span>";
					inHTML +=				"</button>";
					inHTML +=			"</div>";
					inHTML +=			"<div class=\"button-container inline-block\">";
					inHTML +=				"<button type=\"button\" class=\"button-button btn-msg btn-msg-ok btn-prompt\">"
					inHTML +=					"<span class=\"text button-text\">"+$.su.CHAR.OPERATION.OK+"</span>";
					inHTML +=				"</button>";
					inHTML +=			"</div>";
					inHTML +=		"</div>";
					inHTML +=	"</div>";
					break;
				/*case "wait":
					break;*/
				case "show":
					inHTML +=	"<div class=\"button-container msg-btn-container\">";
					inHTML +=		"<div class=\"msg-btn-wrap\">";
					inHTML +=			"<div class=\"button-container inline-block\">";
					inHTML +=				"<button type=\"button\" class=\"button-button btn-msg btn-msg-cancel btn-show\">";
					inHTML +=					"<span class=\"text button-text\">"+$.su.CHAR.OPERATION.CANCEL+"</span>";
					inHTML +=				"</button>";
					inHTML +=			"</div>";
					inHTML +=			"<div class=\"button-container inline-block\">";
					inHTML +=				"<button type=\"button\" class=\"button-button btn-msg btn-msg-no btn-show\">";
					inHTML +=					"<span class=\"text button-text\">"+$.su.CHAR.OPERATION.NO+"</span>";
					inHTML +=				"</button>";
					inHTML +=			"</div>";
					inHTML +=			"<div class=\"button-container inline-block\">";
					inHTML +=				"<button type=\"button\" class=\"button-button btn-msg btn-msg-ok btn-show\">"
					inHTML +=					"<span class=\"text button-text\">"+$.su.CHAR.OPERATION.YES+"</span>";
					inHTML +=				"</button>";
					inHTML +=			"</div>";
					inHTML +=		"</div>";
					inHTML +=	"</div>";
					break;
				/*case "window":
					break;*/
				default:
					var buttonsCfg = this.buttons;
					if (buttonsCfg){
						//这里扩展自定义按钮的情况
					};
					break;
			};

				inHTML +=	"</div>";

			// for ie shadow
				inHTML =		'<div class="position-center-right">'+ inHTML +'</div>';
				inHTML =		'<div class="position-center-left">' + inHTML + '</div>';
				
				inHTML =		'<div class="position-top-right"></div>' + inHTML;
				inHTML =		'<div class="position-top-center"></div>' + inHTML;
				inHTML =		'<div class="position-top-left"></div>' + inHTML;

				inHTML +=		'<div class="position-bottom-left"></div>';
				inHTML +=		'<div class="position-bottom-center"></div>';
				inHTML +=		'<div class="position-bottom-right"></div>';

			var msgBoxsContainer = $("div#msg-boxs-container");
			if (msgBoxsContainer.length == 0){
				msgBoxsContainer = $("<div id=\"msg-boxs-container\" class=\"msg-boxs-container\"></div>");
				$("body").append(msgBoxsContainer);
			};
			msgBoxsContainer.find("#"+obj.id).remove();
			msgBoxsContainer.append(container.detach());

			container.append($(inHTML)).addClass("container widget-container msg-container " + this.cls).css({
				"z-index": "999",
				"display": "none"
			});

			this.isMsg = true;
			$.su.msgManager.add(this);
		});

		me.delegate("a.msg-close", "click", function(e){
			e.preventDefault();

			$(this).closest("div.msg-container").msg("close");
		}).delegate("button.btn-msg-ok", "click", function(e){
			e.preventDefault();

			var container = $(this).closest("div.msg-container");

			if (container.get(0).okHandler){
				if (container.get(0).okHandler.call(me) !== false){
					container.msg("close");
				};
			}else{
				container.msg("close");
			};
			
		}).delegate("button.btn-msg-no", "click", function(e){
			e.preventDefault();

			var container = $(this).closest("div.msg-container");
			container.msg("close", container.get(0).noHandler);

		}).delegate("button.btn-msg-cancel", "click", function(e){
			e.preventDefault();

			var container = $(this).closest("div.msg-container");
			container.msg("close", container.get(0).cancelHandler);
		});

		me.draggable();

		if (me.get(0).autoshow){
			me.msg("show");
		};
		return me;
	},
	show: function(me, arguments){
		var me = me || this;
		var setCenter = arguments[1] || false;

		//if (setCenter){
		me.msg("setPosition", "center", "center");
		//};

		me.each(function(){
			if (this.isMsg === true){
				/*var mask = this.mask;
				if (mask){
					mask.fadeIn(200, function(){
						$(this).css("display", "block");
					});
				};*/
				if (this.mask){
					$.su.mask.show();
				}
				$(this).fadeIn(200, function(){
					me.css("display", "block");
				});
				
			}else{
				//console.error("MSG BOX is not initialed!!!");
				return false;
			};
		});

		me.trigger("ev_show", me);
		return me;
	},
	close: function(me, arguments){	//arguments: _callback, _destroy
		var me = me || this;
		var callback = $.type(arguments[1]) === "function" ? arguments[1] : null,
			destroy = $.type(arguments[1]) === "boolean" ? arguments[1] : $.type(arguments[2]) === "boolean" ? arguments[2] : false;

		$.su.mask.hide();

		if (destroy){
			/*if (me.get(0).mask){
				me.get(0).mask.fadeOut(200, function(){
					$(this).remove();
				});
			};*/
			me.fadeOut(200, function(){
				me.remove();
			});
		}else{
			/*if (me.get(0).mask){
				me.get(0).mask.fadeOut(200, function(){
					$(this).css("display", "none");
				});
			};*/
			me.fadeOut(200, function(){
				me.css("display", "none");
			});
		};
		
		me.trigger("ev_close", me);
		
		if (callback){
			return callback.call(me, me);
		}else{
			return me;
		};
	},
	setTitle: function(me, arguments){ //_title
		var me = me || this,
			obj = me.get(0),
			title = arguments[1];
		if (title){
			obj._title = title;
			me.find("span.msg-title-container").html(title);
		};
		return me;
	},
	setContent: function(me, arguments){ //_content可以是html代码！
		var me = me || this,
			text = arguments[1];
		if (text){
			me.find("span.msg-content-container").html(text);
		};
		return me;
	},
	showButtons: function(me){
		var me = me || this,
			buttonsContainer = me.find("div.msg-btn-container");

		buttonsContainer.fadeIn(150);

		return me;
	},
	hideButtons: function(me){
		var me = me || this,
			buttonsContainer = me.find("div.msg-btn-container");

		buttonsContainer.css("display", "none");

		return me;
	}
});

})(jQuery);