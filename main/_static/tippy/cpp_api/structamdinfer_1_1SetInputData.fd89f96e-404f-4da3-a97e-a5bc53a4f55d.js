selector_to_html = {"a[href=\"#struct-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Struct Documentation<a class=\"headerlink\" href=\"#struct-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#_CPPv4I0ENK8amdinfer12SetInputDataclEvPN4Json5ValueEPv6size_t\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4I0ENK8amdinfer12SetInputDataclEvPN4Json5ValueEPv6size_t\">\n<span id=\"_CPPv3I0ENK8amdinfer12SetInputDataclEPN4Json5ValueEPv6size_t\"></span><span id=\"_CPPv2I0ENK8amdinfer12SetInputDataclEPN4Json5ValueEPv6size_t\"></span><span class=\"k\"><span class=\"pre\">template</span></span><span class=\"p\"><span class=\"pre\">&lt;</span></span><span class=\"k\"><span class=\"pre\">typename</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">T</span></span></span><span class=\"p\"><span class=\"pre\">&gt;</span></span><br/><span class=\"target\" id=\"structamdinfer_1_1SetInputData_1a6fc4489729c9fcffc4256ffc6c17d599\"></span><span class=\"k\"><span class=\"pre\">inline</span></span><span class=\"w\"> </span><span class=\"kt\"><span class=\"pre\">void</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"k\"><span class=\"pre\">operator</span></span><span class=\"o\"><span class=\"pre\">()</span></span></span><span class=\"sig-paren\">(</span><span class=\"n\"><span class=\"pre\">Json</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">Value</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">*</span></span><span class=\"n sig-param\"><span class=\"pre\">json</span></span>, <span class=\"kt\"><span class=\"pre\">void</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">*</span></span><span class=\"n sig-param\"><span class=\"pre\">src_data</span></span>, <span class=\"n\"><span class=\"pre\">size_t</span></span><span class=\"w\"> </span><span class=\"n sig-param\"><span class=\"pre\">src_size</span></span><span class=\"sig-paren\">)</span><span class=\"w\"> </span><span class=\"k\"><span class=\"pre\">const</span></span><br/></dt><dd></dd>", "a[href=\"file__workspace_amdinfer_src_amdinfer_clients_http_internal.hpp.html#file-workspace-amdinfer-src-amdinfer-clients-http-internal-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File http_internal.hpp<a class=\"headerlink\" href=\"#file-http-internal-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_clients.html#dir-workspace-amdinfer-src-amdinfer-clients\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/clients</span></code>)</p><p>Defines the internal objects used for HTTP/REST.</p>", "a[href=\"#struct-setinputdata\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Struct SetInputData<a class=\"headerlink\" href=\"#struct-setinputdata\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Struct Documentation<a class=\"headerlink\" href=\"#struct-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#_CPPv4N8amdinfer12SetInputDataE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer12SetInputDataE\">\n<span id=\"_CPPv3N8amdinfer12SetInputDataE\"></span><span id=\"_CPPv2N8amdinfer12SetInputDataE\"></span><span id=\"amdinfer::SetInputData\"></span><span class=\"target\" id=\"structamdinfer_1_1SetInputData\"></span><span class=\"k\"><span class=\"pre\">struct</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">SetInputData</span></span></span><br/></dt><dd></dd>"}
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
