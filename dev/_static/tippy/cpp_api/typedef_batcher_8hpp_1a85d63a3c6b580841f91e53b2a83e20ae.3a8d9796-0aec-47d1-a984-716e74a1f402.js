selector_to_html = {"a[href=\"#typedef-amdinfer-batchptr\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Typedef amdinfer::BatchPtr<a class=\"headerlink\" href=\"#typedef-amdinfer-batchptr\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Typedef Documentation<a class=\"headerlink\" href=\"#typedef-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"classamdinfer_1_1Batch.html#_CPPv4N8amdinfer5BatchE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer5BatchE\">\n<span id=\"_CPPv3N8amdinfer5BatchE\"></span><span id=\"_CPPv2N8amdinfer5BatchE\"></span><span id=\"amdinfer::Batch\"></span><span class=\"target\" id=\"classamdinfer_1_1Batch\"></span><span class=\"k\"><span class=\"pre\">class</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">Batch</span></span></span><br/></dt><dd><p>The <a class=\"reference internal\" href=\"#classamdinfer_1_1Batch\"><span class=\"std std-ref\">Batch</span></a> is what the batcher produces and pushes to the workers. It represents the requests, the buffers associated with the request and other metadata that should be sent to the worker. </p></dd>", "a[href=\"#typedef-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Typedef Documentation<a class=\"headerlink\" href=\"#typedef-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"file__workspace_amdinfer_src_amdinfer_batching_batcher.hpp.html#file-workspace-amdinfer-src-amdinfer-batching-batcher-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File batcher.hpp<a class=\"headerlink\" href=\"#file-batcher-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_batching.html#dir-workspace-amdinfer-src-amdinfer-batching\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/batching</span></code>)</p><p>Defines the base batcher implementation.</p>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>"}
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
