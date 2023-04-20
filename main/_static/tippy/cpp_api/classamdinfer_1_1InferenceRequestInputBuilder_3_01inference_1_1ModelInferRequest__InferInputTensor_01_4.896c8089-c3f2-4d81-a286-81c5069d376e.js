selector_to_html = {"a[href=\"#class-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Class Documentation<a class=\"headerlink\" href=\"#class-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"file__workspace_amdinfer_src_amdinfer_servers_grpc_server.cpp.html#file-workspace-amdinfer-src-amdinfer-servers-grpc-server-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File grpc_server.cpp<a class=\"headerlink\" href=\"#file-grpc-server-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_servers.html#dir-workspace-amdinfer-src-amdinfer-servers\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/servers</span></code>)</p><p>Implements the gRPC server.</p>", "a[href=\"#template-class-inferencerequestinputbuilder-inference-modelinferrequest-inferinputtensor\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Template Class InferenceRequestInputBuilder&lt; inference::ModelInferRequest_InferInputTensor &gt;<a class=\"headerlink\" href=\"#template-class-inferencerequestinputbuilder-inference-modelinferrequest-inferinputtensor\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Class Documentation<a class=\"headerlink\" href=\"#class-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
