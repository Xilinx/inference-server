selector_to_html = {"a[href=\"#typedef-amdinfer-inferenceresponsefuture\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Typedef amdinfer::InferenceResponseFuture<a class=\"headerlink\" href=\"#typedef-amdinfer-inferenceresponsefuture\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Typedef Documentation<a class=\"headerlink\" href=\"#typedef-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#typedef-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Typedef Documentation<a class=\"headerlink\" href=\"#typedef-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"file__workspace_amdinfer_include_amdinfer_declarations.hpp.html#file-workspace-amdinfer-include-amdinfer-declarations-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File declarations.hpp<a class=\"headerlink\" href=\"#file-declarations-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_include_amdinfer.html#dir-workspace-amdinfer-include-amdinfer\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/include/amdinfer</span></code>)</p>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"classamdinfer_1_1InferenceResponse.html#_CPPv4N8amdinfer17InferenceResponseE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer17InferenceResponseE\">\n<span id=\"_CPPv3N8amdinfer17InferenceResponseE\"></span><span id=\"_CPPv2N8amdinfer17InferenceResponseE\"></span><span id=\"amdinfer::InferenceResponse\"></span><span class=\"target\" id=\"classamdinfer_1_1InferenceResponse\"></span><span class=\"k\"><span class=\"pre\">class</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">InferenceResponse</span></span></span><br/></dt><dd><p>Creates an inference response object based on KServe\u2019s V2 spec that is used to respond back to clients. </p></dd>"}
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
