selector_to_html = {"a[href=\"file__workspace_amdinfer_src_amdinfer_workers_c_plus_plus.cpp.html#file-workspace-amdinfer-src-amdinfer-workers-c-plus-plus-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File c_plus_plus.cpp<a class=\"headerlink\" href=\"#file-c-plus-plus-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_workers.html#dir-workspace-amdinfer-src-amdinfer-workers\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/workers</span></code>)</p><p>Implements the CPlusPlus worker.</p>", "a[href=\"#function-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#function-amdinfer-getfunction\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::getFunction<a class=\"headerlink\" href=\"#function-amdinfer-getfunction\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#_CPPv4N8amdinfer11getFunctionEPvRKNSt6stringE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer11getFunctionEPvRKNSt6stringE\">\n<span id=\"_CPPv3N8amdinfer11getFunctionEPvRKNSt6stringE\"></span><span id=\"_CPPv2N8amdinfer11getFunctionEPvRKNSt6stringE\"></span><span id=\"amdinfer::getFunction__voidP.ssCR\"></span><span class=\"target\" id=\"c__plus__plus_8cpp_1ad50576352aa9fe95a1f335068a150f86\"></span><span class=\"kt\"><span class=\"pre\">void</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">*</span></span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">getFunction</span></span></span><span class=\"sig-paren\">(</span><span class=\"kt\"><span class=\"pre\">void</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">*</span></span><span class=\"n sig-param\"><span class=\"pre\">handle</span></span>, <span class=\"k\"><span class=\"pre\">const</span></span><span class=\"w\"> </span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">string</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">&amp;</span></span><span class=\"n sig-param\"><span class=\"pre\">function</span></span><span class=\"sig-paren\">)</span><br/></dt><dd></dd>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>"}
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
