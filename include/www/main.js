/*global Module:true, $:true, Empebble:true*/
(function(){
  Empebble = {
    create_dict: Module.cwrap("create_dict", "void", []),
    add_string_to_dict: Module.cwrap("add_string_to_dict", "void", ["number", "string"]),
    add_uint8_to_dict: Module.cwrap("add_uint8_to_dict", "void", ["number", "number"]),
    add_uint16_to_dict: Module.cwrap("add_uint16_to_dict", "void", ["number", "number"]),
    add_uint32_to_dict: Module.cwrap("add_uint32_to_dict", "void", ["number", "number"]),
    add_int8_to_dict: Module.cwrap("add_int8_to_dict", "void", ["number", "number"]),
    add_int16_to_dict: Module.cwrap("add_int16_to_dict", "void", ["number", "number"]),
    add_int32_to_dict: Module.cwrap("add_int32_to_dict", "void", ["number", "number"]),
    send_dict_to_pebble: Module.cwrap("send_dict_to_pebble", "void", []),

    add_bytes_to_dict: function(id, bytes) {
      var buf = Module._malloc(bytes.length);
      Module.HEAPU8.set(bytes, buf);
      Module.ccall('add_bytes_to_dict', 'void', ['number', 'number', 'number'],
                   [id, buf, bytes.length]);
      Module._free(buf);
    },

    press_back: Module.cwrap("press_back", "void", []),
    press_up: Module.cwrap("press_up", "void", []),
    press_select: Module.cwrap("press_select", "void", []),
    press_down: Module.cwrap("press_down", "void", [])
  };

  $(window).ready(function() {
    $(".buttons div").on("click", function() {
      var button = this.className.split("-")[0];
      Empebble["press_" + button]();
    });

    var keydown = false;
    $(document).keydown(function(e) {
      var mapping = {37: "back", 38: "up", 39: "select", 40: "down"};
      var button = mapping[e.which];
      if (button != undefined) {
        keydown = true;
        Empebble["press_" + button]();
        $(".buttons ." + button + "-button").addClass("active");
        e.preventDefault();
      }
    });

    $(document).keyup(function(e) {
      if (keydown) {
        $(".buttons div").removeClass("active");
        keydown = false;
      }
    });

  });
})();
