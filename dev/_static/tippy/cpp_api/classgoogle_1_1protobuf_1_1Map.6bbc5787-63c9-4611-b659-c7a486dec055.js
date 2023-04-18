selector_to_html = {"a[href=\"#class-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Class Documentation<a class=\"headerlink\" href=\"#class-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#_CPPv4I00EN6google8protobuf3MapE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4I00EN6google8protobuf3MapE\">\n<span id=\"_CPPv3I00EN6google8protobuf3MapE\"></span><span id=\"_CPPv2I00EN6google8protobuf3MapE\"></span><span class=\"k\"><span class=\"pre\">template</span></span><span class=\"p\"><span class=\"pre\">&lt;</span></span><span class=\"k\"><span class=\"pre\">typename</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">T</span></span></span><span class=\"p\"><span class=\"pre\">,</span></span><span class=\"w\"> </span><span class=\"k\"><span class=\"pre\">typename</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">U</span></span></span><span class=\"p\"><span class=\"pre\">&gt;</span></span><br/><span class=\"target\" id=\"classgoogle_1_1protobuf_1_1Map\"></span><span class=\"k\"><span class=\"pre\">class</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">Map</span></span></span><br/></dt><dd></dd>", "a[href=\"#template-class-map\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Template Class Map<a class=\"headerlink\" href=\"#template-class-map\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Class Documentation<a class=\"headerlink\" href=\"#class-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"file__workspace_amdinfer_src_amdinfer_clients_grpc_internal.hpp.html#file-workspace-amdinfer-src-amdinfer-clients-grpc-internal-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File grpc_internal.hpp<a class=\"headerlink\" href=\"#file-grpc-internal-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_clients.html#dir-workspace-amdinfer-src-amdinfer-clients\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/clients</span></code>)</p><p>Defines the internal objects used for gRPC.</p>"}
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
