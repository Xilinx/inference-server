selector_to_html = {"a[href=\"classamdinfer_1_1Buffer.html#_CPPv4N8amdinfer6BufferE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer6BufferE\">\n<span id=\"_CPPv3N8amdinfer6BufferE\"></span><span id=\"_CPPv2N8amdinfer6BufferE\"></span><span id=\"amdinfer::Buffer\"></span><span class=\"target\" id=\"classamdinfer_1_1Buffer\"></span><span class=\"k\"><span class=\"pre\">class</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">Buffer</span></span></span><br/></dt><dd><p>The base buffer class. <a class=\"reference internal\" href=\"#classamdinfer_1_1Buffer\"><span class=\"std std-ref\">Buffer</span></a> implementations should extend this class and override the methods. </p><p>Subclassed by <a class=\"reference internal\" href=\"classamdinfer_1_1CpuBuffer.html#classamdinfer_1_1CpuBuffer\"><span class=\"std std-ref\">amdinfer::CpuBuffer</span></a>, <a class=\"reference internal\" href=\"classamdinfer_1_1VartTensorBuffer.html#classamdinfer_1_1VartTensorBuffer\"><span class=\"std std-ref\">amdinfer::VartTensorBuffer</span></a>, <a class=\"reference internal\" href=\"classamdinfer_1_1VectorBuffer.html#classamdinfer_1_1VectorBuffer\"><span class=\"std std-ref\">amdinfer::VectorBuffer</span></a></p></dd>", "a[href=\"#typedef-amdinfer-bufferrawptrs\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Typedef amdinfer::BufferRawPtrs<a class=\"headerlink\" href=\"#typedef-amdinfer-bufferrawptrs\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Typedef Documentation<a class=\"headerlink\" href=\"#typedef-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#typedef-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Typedef Documentation<a class=\"headerlink\" href=\"#typedef-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"file__workspace_amdinfer_include_amdinfer_declarations.hpp.html#file-workspace-amdinfer-include-amdinfer-declarations-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File declarations.hpp<a class=\"headerlink\" href=\"#file-declarations-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_include_amdinfer.html#dir-workspace-amdinfer-include-amdinfer\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/include/amdinfer</span></code>)</p>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>"}
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
