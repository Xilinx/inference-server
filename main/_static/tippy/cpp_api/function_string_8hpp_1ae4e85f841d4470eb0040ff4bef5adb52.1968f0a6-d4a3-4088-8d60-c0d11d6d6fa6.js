selector_to_html = {"a[href=\"#function-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#_CPPv4N8amdinfer4util5splitENSt11string_viewENSt11string_viewE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer4util5splitENSt11string_viewENSt11string_viewE\">\n<span id=\"_CPPv3N8amdinfer4util5splitENSt11string_viewENSt11string_viewE\"></span><span id=\"_CPPv2N8amdinfer4util5splitENSt11string_viewENSt11string_viewE\"></span><span id=\"amdinfer::util::split__std::string_view.std::string_view\"></span><span class=\"target\" id=\"string_8hpp_1ae4e85f841d4470eb0040ff4bef5adb52\"></span><span class=\"k\"><span class=\"pre\">inline</span></span><span class=\"w\"> </span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">vector</span></span><span class=\"p\"><span class=\"pre\">&lt;</span></span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">string</span></span><span class=\"p\"><span class=\"pre\">&gt;</span></span><span class=\"w\"> </span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">util</span></span><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">split</span></span></span><span class=\"sig-paren\">(</span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">string_view</span></span><span class=\"w\"> </span><span class=\"n sig-param\"><span class=\"pre\">str</span></span>, <span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">string_view</span></span><span class=\"w\"> </span><span class=\"n sig-param\"><span class=\"pre\">delimiter</span></span><span class=\"sig-paren\">)</span><br/></dt><dd></dd>", "a[href=\"#function-amdinfer-util-split\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::util::split<a class=\"headerlink\" href=\"#function-amdinfer-util-split\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"file__workspace_amdinfer_src_amdinfer_util_string.hpp.html#file-workspace-amdinfer-src-amdinfer-util-string-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File string.hpp<a class=\"headerlink\" href=\"#file-string-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_util.html#dir-workspace-amdinfer-src-amdinfer-util\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/util</span></code>)</p>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>"}
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
