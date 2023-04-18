selector_to_html = {"a[href=\"#class-grpcapiunary\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Class GrpcApiUnary<a class=\"headerlink\" href=\"#class-grpcapiunary\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Inheritance Relationships<a class=\"headerlink\" href=\"#inheritance-relationships\" title=\"Permalink to this heading\">\u00b6</a></h2><h3>Base Type<a class=\"headerlink\" href=\"#base-type\" title=\"Permalink to this heading\">\u00b6</a></h3>", "a[href=\"#inheritance-relationships\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Inheritance Relationships<a class=\"headerlink\" href=\"#inheritance-relationships\" title=\"Permalink to this heading\">\u00b6</a></h2><h3>Base Type<a class=\"headerlink\" href=\"#base-type\" title=\"Permalink to this heading\">\u00b6</a></h3>", "a[href=\"#base-type\"]": "<h3 class=\"tippy-header\" style=\"margin-top: 0;\">Base Type<a class=\"headerlink\" href=\"#base-type\" title=\"Permalink to this heading\">\u00b6</a></h3>", "a[href=\"classamdinfer_1_1Interface.html#exhale-class-classamdinfer-1-1interface\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Class Interface<a class=\"headerlink\" href=\"#class-interface\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Inheritance Relationships<a class=\"headerlink\" href=\"#inheritance-relationships\" title=\"Permalink to this heading\">\u00b6</a></h2><h3>Derived Types<a class=\"headerlink\" href=\"#derived-types\" title=\"Permalink to this heading\">\u00b6</a></h3>", "a[href=\"file__workspace_amdinfer_src_amdinfer_servers_grpc_server.cpp.html#file-workspace-amdinfer-src-amdinfer-servers-grpc-server-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File grpc_server.cpp<a class=\"headerlink\" href=\"#file-grpc-server-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_servers.html#dir-workspace-amdinfer-src-amdinfer-servers\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/servers</span></code>)</p><p>Implements the gRPC server.</p>", "a[href=\"#class-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Class Documentation<a class=\"headerlink\" href=\"#class-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
