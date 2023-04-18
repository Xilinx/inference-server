selector_to_html = {"a[href=\"file__workspace_amdinfer_src_amdinfer_clients_http_internal.hpp.html#file-workspace-amdinfer-src-amdinfer-clients-http-internal-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File http_internal.hpp<a class=\"headerlink\" href=\"#file-http-internal-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_clients.html#dir-workspace-amdinfer-src-amdinfer-clients\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/clients</span></code>)</p><p>Defines the internal objects used for HTTP/REST.</p>", "a[href=\"#class-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Class Documentation<a class=\"headerlink\" href=\"#class-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#template-class-inferencerequestbuilder-std-shared-ptr-json-value\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Template Class InferenceRequestBuilder&lt; std::shared_ptr&lt; Json::Value &gt; &gt;<a class=\"headerlink\" href=\"#template-class-inferencerequestbuilder-std-shared-ptr-json-value\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Class Documentation<a class=\"headerlink\" href=\"#class-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
