selector_to_html = {"a[href=\"#enum-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Enum Documentation<a class=\"headerlink\" href=\"#enum-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#enum-updatecommandtype\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Enum UpdateCommandType<a class=\"headerlink\" href=\"#enum-updatecommandtype\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Enum Documentation<a class=\"headerlink\" href=\"#enum-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"file__workspace_amdinfer_src_amdinfer_core_manager.hpp.html#file-workspace-amdinfer-src-amdinfer-core-manager-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File manager.hpp<a class=\"headerlink\" href=\"#file-manager-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_core.html#dir-workspace-amdinfer-src-amdinfer-core\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/core</span></code>)</p><p>Defines how the shared mutable state is managed.</p>"}
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
