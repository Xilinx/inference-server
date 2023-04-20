selector_to_html = {"a[href=\"classamdinfer_1_1GrpcClient.html#exhale-class-classamdinfer-1-1grpcclient\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Class GrpcClient<a class=\"headerlink\" href=\"#class-grpcclient\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Nested Relationships<a class=\"headerlink\" href=\"#nested-relationships\" title=\"Permalink to this heading\">\u00b6</a></h2><h3>Nested Types<a class=\"headerlink\" href=\"#nested-types\" title=\"Permalink to this heading\">\u00b6</a></h3>", "a[href=\"#nested-relationships\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Nested Relationships<a class=\"headerlink\" href=\"#nested-relationships\" title=\"Permalink to this heading\">\u00b6</a></h2><p>This class is a nested type of <a class=\"reference internal\" href=\"classamdinfer_1_1GrpcClient.html#exhale-class-classamdinfer-1-1grpcclient\"><span class=\"std std-ref\">Class GrpcClient</span></a>.</p>", "a[href=\"#class-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Class Documentation<a class=\"headerlink\" href=\"#class-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"file__workspace_amdinfer_src_amdinfer_clients_grpc.cpp.html#file-workspace-amdinfer-src-amdinfer-clients-grpc-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File grpc.cpp<a class=\"headerlink\" href=\"#file-grpc-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_clients.html#dir-workspace-amdinfer-src-amdinfer-clients\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/clients</span></code>)</p><p>Implements the methods for interacting with the server with gRPC.</p>", "a[href=\"#class-grpcclient-grpcclientimpl\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Class GrpcClient::GrpcClientImpl<a class=\"headerlink\" href=\"#class-grpcclient-grpcclientimpl\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Nested Relationships<a class=\"headerlink\" href=\"#nested-relationships\" title=\"Permalink to this heading\">\u00b6</a></h2><p>This class is a nested type of <a class=\"reference internal\" href=\"classamdinfer_1_1GrpcClient.html#exhale-class-classamdinfer-1-1grpcclient\"><span class=\"std std-ref\">Class GrpcClient</span></a>.</p>"}
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
