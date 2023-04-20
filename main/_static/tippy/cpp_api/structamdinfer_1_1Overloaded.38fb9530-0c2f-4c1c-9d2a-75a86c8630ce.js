selector_to_html = {"a[href=\"#struct-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Struct Documentation<a class=\"headerlink\" href=\"#struct-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#inheritance-relationships\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Inheritance Relationships<a class=\"headerlink\" href=\"#inheritance-relationships\" title=\"Permalink to this heading\">\u00b6</a></h2><h3>Base Type<a class=\"headerlink\" href=\"#base-type\" title=\"Permalink to this heading\">\u00b6</a></h3>", "a[href=\"#base-type\"]": "<h3 class=\"tippy-header\" style=\"margin-top: 0;\">Base Type<a class=\"headerlink\" href=\"#base-type\" title=\"Permalink to this heading\">\u00b6</a></h3>", "a[href=\"file__workspace_amdinfer_src_amdinfer_clients_grpc_internal.cpp.html#file-workspace-amdinfer-src-amdinfer-clients-grpc-internal-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File grpc_internal.cpp<a class=\"headerlink\" href=\"#file-grpc-internal-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_clients.html#dir-workspace-amdinfer-src-amdinfer-clients\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/clients</span></code>)</p><p>Implements the internal objects used for gRPC.</p>", "a[href=\"#_CPPv4IDpEN8amdinfer10OverloadedE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4IDpEN8amdinfer10OverloadedE\">\n<span id=\"_CPPv3IDpEN8amdinfer10OverloadedE\"></span><span id=\"_CPPv2IDpEN8amdinfer10OverloadedE\"></span><span class=\"k\"><span class=\"pre\">template</span></span><span class=\"p\"><span class=\"pre\">&lt;</span></span><span class=\"k\"><span class=\"pre\">class</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">...</span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">Ts</span></span></span><span class=\"p\"><span class=\"pre\">&gt;</span></span><br/><span class=\"target\" id=\"structamdinfer_1_1Overloaded\"></span><span class=\"k\"><span class=\"pre\">struct</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">Overloaded</span></span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">:</span></span><span class=\"w\"> </span><span class=\"k\"><span class=\"pre\">public</span></span><span class=\"w\"> </span><a class=\"reference internal\" href=\"#_CPPv4IDpEN8amdinfer10OverloadedE\" title=\"amdinfer::Overloaded::Ts\"><span class=\"n\"><span class=\"pre\">Ts</span></span></a><br/></dt><dd></dd>", "a[href=\"#template-struct-overloaded\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Template Struct Overloaded<a class=\"headerlink\" href=\"#template-struct-overloaded\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Inheritance Relationships<a class=\"headerlink\" href=\"#inheritance-relationships\" title=\"Permalink to this heading\">\u00b6</a></h2><h3>Base Type<a class=\"headerlink\" href=\"#base-type\" title=\"Permalink to this heading\">\u00b6</a></h3>"}
skip_classes = ["headerlink", "sd-stretched-link"]

window.onload = function () {
    for (const [select, tip_html] of Object.entries(selector_to_html)) {
        const links = document.querySelectorAll(` ${select}`);
        for (const link of links) {
            if (skip_classes.some(c => link.classList.contains(c))) {
                continue;
            }

            tippy(link, {
                content: tip_html,
                allowHTML: true,
                arrow: true,
                placement: 'auto-start', maxWidth: 500, interactive: false,

            });
        };
    };
    console.log("tippy tips loaded!");
};
