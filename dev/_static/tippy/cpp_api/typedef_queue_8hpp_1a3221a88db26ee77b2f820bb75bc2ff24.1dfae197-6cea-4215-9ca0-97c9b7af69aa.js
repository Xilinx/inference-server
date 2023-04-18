selector_to_html = {"a[href=\"#_CPPv4I0EN8amdinfer13BlockingQueueE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4I0EN8amdinfer13BlockingQueueE\">\n<span id=\"_CPPv3I0EN8amdinfer13BlockingQueueE\"></span><span id=\"_CPPv2I0EN8amdinfer13BlockingQueueE\"></span><span class=\"k\"><span class=\"pre\">template</span></span><span class=\"p\"><span class=\"pre\">&lt;</span></span><span class=\"k\"><span class=\"pre\">class</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">T</span></span></span><span class=\"p\"><span class=\"pre\">&gt;</span></span><br/><span class=\"target\" id=\"queue_8hpp_1a3221a88db26ee77b2f820bb75bc2ff24\"></span><span class=\"k\"><span class=\"pre\">using</span></span><span class=\"w\"> </span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">BlockingQueue</span></span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">=</span></span><span class=\"w\"> </span><span class=\"n\"><span class=\"pre\">moodycamel</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">BlockingConcurrentQueue</span></span><span class=\"p\"><span class=\"pre\">&lt;</span></span><a class=\"reference internal\" href=\"#_CPPv4I0EN8amdinfer13BlockingQueueE\" title=\"amdinfer::BlockingQueue::T\"><span class=\"n\"><span class=\"pre\">T</span></span></a><span class=\"p\"><span class=\"pre\">&gt;</span></span><br/></dt><dd></dd>", "a[href=\"#typedef-amdinfer-blockingqueue\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Typedef amdinfer::BlockingQueue<a class=\"headerlink\" href=\"#typedef-amdinfer-blockingqueue\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Typedef Documentation<a class=\"headerlink\" href=\"#typedef-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#typedef-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Typedef Documentation<a class=\"headerlink\" href=\"#typedef-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"file__workspace_amdinfer_src_amdinfer_util_queue.hpp.html#file-workspace-amdinfer-src-amdinfer-util-queue-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File queue.hpp<a class=\"headerlink\" href=\"#file-queue-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_util.html#dir-workspace-amdinfer-src-amdinfer-util\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/util</span></code>)</p>"}
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
