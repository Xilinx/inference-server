selector_to_html = {"a[href=\"#function-amdinfer-util-setthreadname-const-char\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::util::setThreadName(const char *)<a class=\"headerlink\" href=\"#function-amdinfer-util-setthreadname-const-char\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#function-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"#_CPPv4N8amdinfer4util13setThreadNameEPKc\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer4util13setThreadNameEPKc\">\n<span id=\"_CPPv3N8amdinfer4util13setThreadNameEPKc\"></span><span id=\"_CPPv2N8amdinfer4util13setThreadNameEPKc\"></span><span id=\"amdinfer::util::setThreadName__cCP\"></span><span class=\"target\" id=\"thread_8hpp_1a1f755ec0f69754bbf457b2c7f68a16c7\"></span><span class=\"k\"><span class=\"pre\">inline</span></span><span class=\"w\"> </span><span class=\"kt\"><span class=\"pre\">void</span></span><span class=\"w\"> </span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">util</span></span><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">setThreadName</span></span></span><span class=\"sig-paren\">(</span><span class=\"k\"><span class=\"pre\">const</span></span><span class=\"w\"> </span><span class=\"kt\"><span class=\"pre\">char</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">*</span></span><span class=\"n sig-param\"><span class=\"pre\">name</span></span><span class=\"sig-paren\">)</span><br/></dt><dd><p>Attempt to set the calling thread\u2019s name. Note, this may or may not succeed. If renaming is not possible, it should silently fail without error. There is some discussion on SO about a cross-platform implementation. </p></dd>", "a[href=\"file__workspace_amdinfer_src_amdinfer_util_thread.hpp.html#file-workspace-amdinfer-src-amdinfer-util-thread-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File thread.hpp<a class=\"headerlink\" href=\"#file-thread-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_util.html#dir-workspace-amdinfer-src-amdinfer-util\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/util</span></code>)</p>"}
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
