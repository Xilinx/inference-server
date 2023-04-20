selector_to_html = {"a[href=\"#_CPPv4N8amdinfer23initializeServerLoggingEv\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer23initializeServerLoggingEv\">\n<span id=\"_CPPv3N8amdinfer23initializeServerLoggingEv\"></span><span id=\"_CPPv2N8amdinfer23initializeServerLoggingEv\"></span><span id=\"amdinfer::initializeServerLogging\"></span><span class=\"target\" id=\"server_8cpp_1acea43796a83b9ee6de28c969539154a4\"></span><span class=\"kt\"><span class=\"pre\">void</span></span><span class=\"w\"> </span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">initializeServerLogging</span></span></span><span class=\"sig-paren\">(</span><span class=\"sig-paren\">)</span><br/></dt><dd></dd>", "a[href=\"#function-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"#function-amdinfer-initializeserverlogging\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::initializeServerLogging<a class=\"headerlink\" href=\"#function-amdinfer-initializeserverlogging\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"file__workspace_amdinfer_src_amdinfer_servers_server.cpp.html#file-workspace-amdinfer-src-amdinfer-servers-server-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File server.cpp<a class=\"headerlink\" href=\"#file-server-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_servers.html#dir-workspace-amdinfer-src-amdinfer-servers\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/servers</span></code>)</p>"}
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
