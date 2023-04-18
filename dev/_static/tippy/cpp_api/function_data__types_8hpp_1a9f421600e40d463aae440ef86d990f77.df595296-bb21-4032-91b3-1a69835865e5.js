selector_to_html = {"a[href=\"#_CPPv4N8amdinfer6detail4hashENSt11string_viewE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer6detail4hashENSt11string_viewE\">\n<span id=\"_CPPv3N8amdinfer6detail4hashENSt11string_viewE\"></span><span id=\"_CPPv2N8amdinfer6detail4hashENSt11string_viewE\"></span><span id=\"amdinfer::detail::hash__std::string_viewCE\"></span><span class=\"target\" id=\"data__types_8hpp_1a9f421600e40d463aae440ef86d990f77\"></span><span class=\"k\"><span class=\"pre\">constexpr</span></span><span class=\"w\"> </span><span class=\"n\"><span class=\"pre\">uint64_t</span></span><span class=\"w\"> </span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">detail</span></span><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">hash</span></span></span><span class=\"sig-paren\">(</span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">string_view</span></span><span class=\"w\"> </span><span class=\"n sig-param\"><span class=\"pre\">str</span></span><span class=\"sig-paren\">)</span><br/></dt><dd></dd>", "a[href=\"file__workspace_amdinfer_include_amdinfer_core_data_types.hpp.html#file-workspace-amdinfer-include-amdinfer-core-data-types-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File data_types.hpp<a class=\"headerlink\" href=\"#file-data-types-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_include_amdinfer_core.html#dir-workspace-amdinfer-include-amdinfer-core\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/include/amdinfer/core</span></code>)</p><p>Defines the used data types.</p>", "a[href=\"#function-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"#function-amdinfer-detail-hash\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::detail::hash<a class=\"headerlink\" href=\"#function-amdinfer-detail-hash\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
