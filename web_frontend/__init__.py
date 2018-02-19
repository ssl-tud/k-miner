from flask import Flask, render_template, jsonify, request, redirect, url_for
from collections import OrderedDict
from json import JSONEncoder
import json
import pickle
import jsonpickle
import os
import datetime
import pygal
from pygal.style import Style
from KernelReportParser import parse_kernel_report
from Report import Comment, PortableMemoryLeak, Var, Context, MemCrptVuln

app = Flask(__name__)

file_prefix = "KernelReport_"
kernel_versions = ["v4.12"]
# kernel_versions = ["v3.19", "v4.2", "v4.6", "v4.10"]
kernel_reports = dict()
kernel_overview_graph = None
kernel_context_graph = None
kernel_schedule_graph = None
kernel_mem_usage_graph = None
syscall_to_kernel_report = OrderedDict()
driver_to_kernel_report = OrderedDict()
mem_crpt_only = list()
memory_leaks = OrderedDict(list())

cur_kernel_version = None
cur_report = None
MEM_CRPT_ONLY = False


class PythonObjectEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, (list, dict, str, int, float, bool, type(None))):
            return JSONEncoder.default(self, obj)
        return {"_python_object": pickle.dumps(obj)}


def as_python_object(dct):
    if "_python_object" in dct:
        return pickle.loads(str(dct["_python_object"]))
    return dct


def version_sorter(v):
    first_num, second_num = v[0].split(".")

    if int(second_num) < 10:
        second_num = "0" + second_num

    return (first_num + second_num, v[1])


def import_comments():
    """
    Imports the comments for all the reports.
    """

    file_name = Comment.get_file_name()
    data = []

    if not os.path.exists(file_name):
        open(file_name, "w+", encoding="utf-8")

    with open(file_name, encoding="utf-8") as f:
        for line in f:
            data.append(json.loads(line))

    if len(data) == 0:
        return

    for json_comments in data[0]:
        raw_c = json.loads(json_comments)
        comment = Comment(raw_c["ID"], raw_c["text"], raw_c["date"])
        kernel_report = kernel_reports[comment.get_kernel_version()]
        report = kernel_report.reports[comment.get_systemcall()]
        report.add_comment(comment)


def export_comments():
    """
    Exports the comments of all the reports.
    """

    file_name = Comment.get_file_name()
    feeds = list()

    with open(file_name, mode='w', encoding='utf-8') as f:
        json.dump([], f)

    for kr in kernel_reports.values():
        for sr in kr.reports.values():
            for c in sr.comments:
                entry = json.dumps(c.__dict__)
                feeds.append(entry)

    with open(file_name, mode='w', encoding='utf-8') as feedsjson:
        json.dump(feeds, feedsjson)


def import_memory_leaks():
    """
    Imports the memory leaks.
    """

    global memory_leaks
    global cur_kernel_report

    file_name = PortableMemoryLeak.get_file_name()
    data = []

    if not os.path.exists(file_name):
        open(file_name, "w+", encoding="utf-8")

    with open(file_name, encoding="utf-8") as f:
        for line in f:
            data.append(json.loads(line))

    if len(data) != 0:
        for json_memory_leaks in data[0]:
            raw_c = json.loads(json_memory_leaks, object_hook=as_python_object)
            cxt = Context(raw_c["loc_function_name"], raw_c["loc_file_name"], raw_c["loc_line"])
            dangling_ptr = Var(raw_c["ptr_name"], cxt)
            leak = PortableMemoryLeak(raw_c["type"], dangling_ptr, raw_c["is_leak"])
            memory_leaks.setdefault(leak.type, list()).append(leak)

    # load new reports if any exist.
    for kr in kernel_reports.values():
        for sr in kr.reports.values():
            for l in sr.uar_checker_report.memory_leaks:
                pml = PortableMemoryLeak("UseAfterReturn", l.local_var)
                lst = memory_leaks.setdefault("UseAfterReturn", [])
                if pml not in lst:
                    lst.append(pml)
            for l in sr.uaf_checker_report.memory_leaks:
                pml = PortableMemoryLeak("UseAfterFree", l.dangling_ptr)
                lst = memory_leaks.setdefault("UseAfterFree", [])
                if pml not in lst:
                    lst.append(pml)
            for l in sr.dfree_checker_report.memory_leaks:
                pml = PortableMemoryLeak("DoubleFree", l.dangling_ptr)
                lst = memory_leaks.setdefault("DoubleFree", [])
                if pml not in lst:
                    lst.append(pml)

    export_memory_leaks()


def export_memory_leaks():
    """
    Exports the memory leaks.
    """

    global memory_leaks

    file_name = PortableMemoryLeak.get_file_name()
    feeds = list()

    with open(file_name, mode='w', encoding='utf-8') as f:
        json.dump([], f)

    for ty, leaks in memory_leaks.items():
        for l in leaks:
            # entry = json.dumps(l, cls=PythonObjectEncoder)
            entry = json.dumps(l.as_dict())
            feeds.append(entry)

    with open(file_name, mode='w', encoding='utf-8') as feedsjson:
        json.dump(feeds, feedsjson)


def import_kernel_reports():
    """
    Imports the kernel reports.
    """

    global kernel_reports
    global kernel_versions
    global syscall_to_kernel_report
    global driver_to_kernel_report

    tmp_versions = list()
    kernel_report = None

    for kv in kernel_versions:
        file_name = file_prefix + kv

        if not os.path.exists(file_name + ".json"):
            open(file_name, "w+", encoding="utf-8")
            # Try to parse the reports from the report file.
            kernel_report = parse_kernel_report(file_name + ".log")
        else:
            with open(file_name + ".json", encoding="utf-8") as f:
                json_str = f.read()

                if json_str == "":
                    continue

                kernel_report = jsonpickle.decode(json_str)

        if kernel_report:
            kernel_reports[kv] = kernel_report
            tmp_versions.append(kv)

            for name, report in kernel_report.reports.items():
                if report.is_syscall_report():
                    syscall_to_kernel_report.setdefault(name, []).append(kernel_report)
                else:
                    driver_to_kernel_report.setdefault(name, []).append(kernel_report)

                if report.get_num_bugs() > 0:
                    mem_crpt_only.append(name)

    kernel_reports = OrderedDict(sorted(kernel_reports.items(), key=version_sorter))

    kernel_versions = list(kernel_reports.keys())
    MemCrptVuln.TOTAL_NUM_IDs = len(kernel_versions)

    print("Imported: " + str(kernel_versions))


def export_kernel_reports():
    """
    Exports the kernel reports.
    """

    global kernel_reports

    for kv, kr in kernel_reports.items():
        with open(kr.get_file_name(), mode='w', encoding='utf-8') as f:
            json_obj = jsonpickle.encode(kr)
            f.write(json_obj)


def update_kernel_report(kernel_report):
    """
    Update a kernel report.
    """

    with open(kernel_report.get_file_name(), mode='w', encoding='utf-8') as f:
        json_obj = jsonpickle.encode(kernel_report)
        f.write(json_obj)


def remove_non_checked_mem_crpt():
    """
    Removes all unchecked reports.
    """

    remaining_mem_crpt = list()

    for kv, kr in kernel_reports.items():
        print("Kernel Version: " + kv)
        mem_crpt_map = kr.get_memory_corruptions().items()

        for lt, mem_crpt in mem_crpt_map:
            for l in mem_crpt:
                    if not l.is_not_checked():
                        remaining_mem_crpt.append(l)
            mem_crpt[:] = remaining_mem_crpt
            remaining_mem_crpt[:] = list()

        update_kernel_report(kr)


def remove_false_positive():
    for kv, kr in kernel_reports.items():
        print("Kernel Version: " + kv)
        mem_crpt_map = kr.get_memory_corruptions()
        remaining_bugs = list()

        for name, r in kr.reports.items():
            for b in r.uar_checker_report.bugs:
                if b in mem_crpt_map["UAR"] and b not in remaining_bugs:
                    remaining_bugs.append(b)
            r.uar_checker_report.bugs[:] = remaining_bugs
            remaining_bugs[:] = list()
            for b in r.dfree_checker_report.bugs:
                if b in mem_crpt_map["DFREE"] and b not in remaining_bugs:
                    remaining_bugs.append(b)
            r.dfree_checker_report.bugs[:] = remaining_bugs
            remaining_bugs[:] = list()
            for b in r.leak_checker_report.bugs:
                if b in mem_crpt_map["LEAK"] and b not in remaining_bugs:
                    remaining_bugs.append(b)
            r.leak_checker_report.bugs[:] = remaining_bugs
            remaining_bugs[:] = list()

        update_kernel_report(kr)


def init_kernel_reports():
    """
    Creates all the kernel reports of the given kernel versions.
    """

    global kernel_versions
    global kernel_reports
    global cur_report

    import_kernel_reports()
    # TODO
    # import_comments()
    # export_kernel_reports()
    # remove_non_checked_mem_crpt()
    # remove_false_positive()


def create_overview_kernel_graph():
    """
    Creates a graph that shows an overview of all different leaks found.
    """

    global kernel_overview_graph
    global kernel_reports

    use_after_return = list()
    use_after_free = list()
    double_free = list()
    memory_leak = list()
    double_lock = list()

    for kv in kernel_versions:
        kr = kernel_reports[kv]
        uar_set = set()
        uaf_set = set()
        dfree_set = set()
        leak_set = set()
        dlock_set = set()

        for sn, sr in kr.reports.items():
            for b in sr.uar_checker_report.bugs:
                uar_set.add(b)
            for b in sr.uaf_checker_report.bugs:
                uaf_set.add(b)
            for b in sr.dfree_checker_report.bugs:
                dfree_set.add(b)
            for b in sr.leak_checker_report.bugs:
                leak_set.add(b)
            for b in sr.dlock_checker_report.bugs:
                dlock_set.add(b)

        use_after_return.append(len(uar_set))
        use_after_free.append(len(uaf_set))
        double_free.append(len(dfree_set))
        memory_leak.append(len(leak_set))
        double_lock.append(len(dlock_set))

    graph_style = Style(colors=('#aaaaaa', '#666666', '#222222'), background='#ffffff')
    line_chart = pygal.Bar(style=graph_style, width=800, height=400)
    line_chart.x_labels = kernel_versions
    line_chart.add('UseAfterFree', use_after_free)
    line_chart.add('UseAfterReturn', use_after_return)
    line_chart.add('DoubleFree', double_free)
    line_chart.add('MemoryLeak', memory_leak)
    line_chart.add('DoubleLock', double_lock)

    kernel_overview_graph = line_chart.render_data_uri()


def create_kernel_context_graph():
    """
    Creates a graph that shows number of functions and global variables of a kernelversion.
    """

    global kernel_context_graph
    global kernel_reports

    num_functions = list()
    num_globalvars = list()

    for kv in kernel_versions:
        kr = kernel_reports[kv]

        if not len(kr.reports) > 0:
            continue

        r = list(kr.reports.values())[0]

        num_functions.append(r.partitioner_stat.numKOFuncs)
        num_globalvars.append(r.partitioner_stat.numKOGVs)

    graph_style = Style(colors=('#aaaaaa', '#666666', '#222222'), background='#ffffff')
    line_chart = pygal.Bar(style=graph_style, width=800, height=400)
    line_chart.x_labels = kernel_versions
    line_chart.add('Functions', num_functions)
    line_chart.add('Global Variables', num_globalvars)

    kernel_context_graph = line_chart.render_data_uri()


def create_kernel_reduction_graph(report):
    """
    Creates a graph of the minimizer statistic of the kernel of the given API.
    """

    graph_style = Style(colors=('#222222', '#666666', '#aaaaaa'), background='#ffffff')
    line_chart = pygal.HorizontalBar(style=graph_style, width=800, height=400)

    # It might be that the report is incomplete.
    if not report.partitioner_stat:
        return line_chart.render_data_uri()

    numKOFuncs = report.partitioner_stat.numKOFuncs
    numKRFuncs = report.partitioner_stat.numKRFuncs
    numKOGVs = report.partitioner_stat.numKOGVs
    numKRGVs = report.partitioner_stat.numKRGVs

    line_chart.x_labels = ["Global Variables", "Functions"]
    line_chart.add('Original', [numKOGVs, numKOFuncs])
    line_chart.add('Relevant', [numKRGVs, numKRFuncs])

    return line_chart.render_data_uri()


def create_schedule_graph():
    """
    Creates a graph of the schedule for all API functions.
    """

    global kernel_schedule_graph
    global cur_kernel_report

    schedule = OrderedDict(list())
    cur_kernel_version = cur_kernel_report.version
    api = kernel_reports[cur_kernel_version].reports.items()
    used_api = list()

    for sn, sr in api:
        schedule_map = sr.get_schedule_map()
        if float(sr.total_duration) > float(cur_kernel_report.get_average_duration()):
            used_api.append(sn)
            context_reduction_t = round(float(sr.total_duration) -
                                        (float(schedule_map["UAR"]) + float(schedule_map["LEAK"]) +
                                         float(schedule_map["DFREE"])))
            schedule.setdefault("context_reduction", []).append(context_reduction_t)
            schedule.setdefault("dfree_check", []).append(float(schedule_map["DFREE"]))
            schedule.setdefault("leak_check", []).append(float(schedule_map["LEAK"]))
            schedule.setdefault("uar_check", []).append(float(schedule_map["UAR"]))

    graph_style = Style(colors=('#222222', '#666666', '#aaaaaa'), background='#ffffff', legend_font_size=30, label_font_size=30)
    line_chart = pygal.StackedBar(style=graph_style, legend_at_bottom=True, x_label_rotation=60)
    line_chart.x_labels = used_api
    line_chart.add('Context Reduction', schedule["context_reduction"])
    line_chart.add('Use-After-Return Checker', schedule["uar_check"])
    line_chart.add('Memory Leak Checker', schedule["leak_check"])
    line_chart.add('Double Free Checker', schedule["dfree_check"])

    kernel_schedule_graph = line_chart.render_data_uri(width=6000, height=1800)
    # line_chart.render_to_file("kernel_schedule_graph.svg")
    line_chart.render_to_png(filename="kernel_schedule_graph.png", width=3000, height=800)


def create_mem_usage_graph():
    """
    Creates a graph of the memory usage for all API functions.
    """
    global kernel_mem_usage_graph
    global cur_kernel_report

    mem_usage_dict = OrderedDict(list())
    cur_kernel_version = "v4.2"
    # cur_kernel_report.version
    api = kernel_reports[cur_kernel_version].reports.items()
    used_api = list()

    for sn, sr in api:
        # average = sr.get_mem_usage()
        max = sr.get_mem_max_usage()

        if int(max)/ 1000 > 11000:
            used_api.append(sn)
            # mem_usage_dict.setdefault("average", []).append(int(average)/1000)
            mem_usage_dict.setdefault("max", []).append(int(max) / 1000)

    graph_style = Style(colors=('#222222', '#666666', '#aaaaaa'), background='#ffffff', legend_font_size=30, label_font_size=30)
    line_chart = pygal.StackedBar(style=graph_style, legend_at_bottom=True, x_label_rotation=60)
    line_chart.x_labels = used_api
    # line_chart.add('Average Memory Usage', mem_usage_dict["average"])
    line_chart.add('Max Memory Usage', mem_usage_dict["max"])

    # kernel_mem_usage_graph = line_chart.render_data_uri(width=6000, height=1800)
    line_chart.render_to_png(filename="kernel_mem_usage_graph.png", width=3000, height=800)


@app.route('/_search/', methods=["GET"])
def _search():
    """
    Helper function that manage the search function.
    """

    global syscall_to_kernel_report
    global driver_to_kernel_report

    wanted_text = request.args.get("search")

    if wanted_text in syscall_to_kernel_report.keys():
        return redirect(url_for("syscall_report", kernel_version=cur_kernel_report.version, syscall_name=wanted_text))

    if wanted_text in driver_to_kernel_report.keys():
        return redirect(url_for("driver_report", kernel_version=cur_kernel_report.version, driver_name=wanted_text))

    return redirect(url_for("search_results", wanted=wanted_text))


@app.route('/mem_crpt_overview/_update_status/', methods=["GET"])
def _mem_crpt_overview_update_status():
    """
    Helper function that handles a check of a leak.
    """

    global kernel_reports

    kernel_version = request.args.get("kernel_version")
    memory_corruption_id = request.args.get("memory_corruption_id")
    status = request.args.get("status")

    print("kernelV= " + kernel_version + " mem_corruption_id= " + memory_corruption_id)

    kernel_report = kernel_reports[kernel_version]
    mem_crpt_map = kernel_report.get_memory_corruptions().items()

    for lt, mem_crpt in mem_crpt_map:
        for l in mem_crpt:
            if l.ID == int(memory_corruption_id):
                l.set_status(status)

                if l.is_removed():
                    kernel_report.delete_memory_corruption(l)

                update_kernel_report(kernel_report)
                return redirect(url_for("mem_crpt_overview"))

    assert(False)


@app.route('/_get_mem_crpt_only/', methods=["GET"])
def _get_mem_crpt_only():
    """
    Helper function that returns a list of all API functions that have at least on leak.
    """
    global mem_crpt_only
    global MEM_CRPT_ONLY

    res = request.args.get("leak_only")
    print("RES = " + res)
    MEM_CRPT_ONLY = (request.args.get("leak_only") != "true")

    return jsonify(result=mem_crpt_only)


@app.route('/report/_prev/', methods=["GET"])
def _report_prev():
    """
    Helper function that returns the prev API function.
    """

    global cur_kernel_report
    global cur_report
    global MEM_CRPT_ONLY

    try:
        cur_kernel_report
    except Exception as e:
        cur_kernel_report = list(kernel_reports.values())[0]

    print("Leak only is " + str(MEM_CRPT_ONLY))
    cur_report = cur_kernel_report.get_prev_report(cur_report, MEM_CRPT_ONLY)

    if not cur_report:
        return render_template("err.html", err_msg="'prev' API function failed!")

    return redirect(url_for("report", kernel_version=cur_kernel_report.version, syscall_name=cur_report.syscall_name))


@app.route('/report/_next/', methods=["GET"])
def _report_next():
    """
    Helper function that returns the next API function.
    """

    global cur_kernel_report
    global cur_report
    global MEM_CRPT_ONLY

    try:
        cur_kernel_report
    except Exception as e:
        cur_kernel_report = list(kernel_reports.values())[0]

    print("Leak only is " + str(MEM_CRPT_ONLY))
    cur_report = cur_kernel_report.get_next_report(cur_report, MEM_CRPT_ONLY)

    if not cur_report:
        return render_template("err.html", err_msg="'next' API function failed!")

    return redirect(url_for("report", kernel_version=cur_kernel_report.version, report_id=cur_report.getID()))


@app.route('/report/_add_comment/', methods=["POST"])
def comments():
    """
    Helper function that creates a comment obj for a report.
    """

    global cur_kernel_report
    global cur_report

    now = datetime.datetime.now()
    id = "" + cur_kernel_report.version + ":" + cur_report.syscall_name + ":" + str(len(cur_report.comments))
    new_comment_text = request.form["new_comment"]

    new_comment = Comment(id, new_comment_text, str(now))

    cur_report.add_comment(new_comment)
    # export_comments()

    return redirect(url_for("report", kernel_version=cur_kernel_report.version, report_id=cur_report.getID()))


@app.route('/kernel_schedule/', methods=["GET"])
def kernel_schedule():
    """
    Shows the results context reduction.
    """

    global cur_kernel_report
    global kernel_schedule_graph

    new_kernel_version = request.args.get("kernel_version")

    if new_kernel_version not in kernel_reports.keys():
        return render_template("err.html", err_msg="kernel version '" + new_kernel_version + "' not found!")

    cur_kernel_report = kernel_reports[new_kernel_version]

    create_schedule_graph()

    graph_data = {"kernel_schedule_graph": kernel_schedule_graph}

    return render_template("kernel_schedule.html", kernel_versions=kernel_versions,
                           cur_kernel_report=cur_kernel_report, graph_data=graph_data)


@app.route('/search_results/', methods=["GET"])
def search_results():
    """
    Shows the results of the search.
    """

    global kernel_versions
    global cur_kernel_report
    global cur_report
    global syscall_to_kernel_report
    global driver_to_kernel_report

    try:
        cur_kernel_report
    except Exception as e:
        cur_kernel_report = list(kernel_reports.values())[0]

    wanted = request.args.get("wanted")

    syscall_to_kernel_res = OrderedDict()
    driver_to_kernel_res = OrderedDict()

    for sn in syscall_to_kernel_report.keys():
        for kr in syscall_to_kernel_report[sn]:
            if kr.reports[sn].has_leak(wanted):
                syscall_to_kernel_res[sn] = syscall_to_kernel_report[sn]
                break

    for dn in driver_to_kernel_report.keys():
        for kr in driver_to_kernel_report[dn]:
            if kr.reports[dn].has_leak(wanted):
                driver_to_kernel_res[dn] = driver_to_kernel_report[dn]
                break

    if len(syscall_to_kernel_res) == 0 and len(driver_to_kernel_res) == 0:
        return render_template("err.html", err_msg="Nothing found with the name '" + wanted + "'!")

    return render_template("search_results.html", syscall_to_kernel_report=syscall_to_kernel_res,
                           driver_to_kernel_report=driver_to_kernel_res,
                           kernel_versions=kernel_versions,
                           cur_kernel_report=cur_kernel_report,
                           cur_report=cur_report, wanted=wanted)


@app.route('/mem_crpt_overview/', methods=["GET"])
def mem_crpt_overview():
    """
    Shows all MemoryLeaks found.
    """

    global kernel_versions
    global kernel_reports
    global cur_kernel_report
    global cur_report

    try:
        cur_kernel_report
    except Exception as e:
        cur_kernel_report = list(kernel_reports.values())[0]

    return render_template("mem_crpt_overview.html", kernel_reports=kernel_reports,
                           kernel_versions=kernel_versions,
                           cur_kernel_report=cur_kernel_report,
                           cur_report=cur_report)


@app.route('/report/', methods=["GET"])
def report():
    """
    Switchs between syscall and driver report.
    """

    global cur_kernel_report
    global cur_report

    # new_kernel_version = request.args.get("kernel_version")
    # new_api_name = request.args.get("report_id")

    try:
        cur_kernel_report
    except Exception as e:
        cur_kernel_report = list(kernel_reports.values())[0]

    if not cur_report:
        return render_template("err.html", err_msg="Report failed!")

    if cur_report.is_driver_report():
        return redirect(url_for("report/driver_report", kernel_version=cur_kernel_report.version, driver_name=cur_report.driver_name))

    return redirect(url_for("syscall_report", kernel_version=cur_kernel_report.version, syscall_name=cur_report.syscall_name))


@app.route('/report/_syscall/', methods=["GET"])
def syscall_report():
    """
    Renders the report page of a given system call in a specific kernel version.
    """

    global cur_kernel_report
    global cur_report

    new_kernel_version = request.args.get("kernel_version")
    new_api_name = request.args.get("syscall_name")

    print("LOAD REPORT")

    if new_kernel_version not in kernel_reports.keys():
        return render_template("err.html", err_msg="kernel version '" + new_kernel_version + "' not found!")

    cur_kernel_report = kernel_reports[new_kernel_version]

    if new_api_name not in cur_kernel_report.reports.keys():
        return render_template("err.html", err_msg="system call '" + new_api_name + "' not found in " + str(cur_kernel_report.version) + "!")

    cur_report = cur_kernel_report.reports[new_api_name]

    if not cur_report:
        return render_template("err.html", err_msg="system call '" + new_api_name + "' not found in " + str(cur_kernel_report.version) + "!")

    kernel_reduction_graph = create_kernel_reduction_graph(cur_report)

    return render_template("syscall_report.html", kernel_versions=kernel_versions, cur_kernel_report=cur_kernel_report, cur_report=cur_report, graph_data=kernel_reduction_graph)


@app.route('/report/_driver/', methods=["GET"])
def driver_report():
    """
    Renders the report page of a given driver in a specific kernel version.
    """

    global cur_kernel_report
    global cur_report

    new_kernel_version = request.args.get("kernel_version")
    new_driver_name = request.args.get("driver_name")

    print("LOAD REPORT")

    if new_kernel_version not in kernel_reports.keys():
        return render_template("err.html", err_msg="kernel version '" + new_kernel_version + "' not found!")

    cur_kernel_report = kernel_reports[new_kernel_version]

    if new_driver_name not in cur_kernel_report.reports.keys():
        return render_template("err.html", err_msg="driver '" + new_driver_name + "' not found in " + str(cur_kernel_report.version) + "!")

    cur_report = cur_kernel_report.reports[new_driver_name]

    if not cur_report:
        return render_template("err.html", err_msg="driver '" + new_driver_name + "' not found in " + str(cur_kernel_report.version) + "!")

    kernel_reduction_graph = create_kernel_reduction_graph(cur_report)

    return render_template("driver_report.html", kernel_versions=kernel_versions, cur_kernel_report=cur_kernel_report, cur_report=cur_report, graph_data=kernel_reduction_graph)


@app.route('/overview/')
def overview():
    """
    Shows an overview of all leaks found.
    """

    global cur_kernel_report
    global cur_report
    global kernel_reports
    global syscall_to_kernel_report
    global driver_to_kernel_report

    # new_kernel_version = request.args.get("kernel_version")
    # cur_kernel_report = kernel_reports[new_kernel_version]

    try:
        cur_kernel_report
    except Exception as e:
        cur_kernel_report = list(kernel_reports.values())[0]

    graph_data = {"kernel_overview_graph": kernel_overview_graph, "kernel_context_graph": kernel_context_graph}

    return render_template("overview.html", syscall_to_kernel_report=syscall_to_kernel_report,
                           driver_to_kernel_report=driver_to_kernel_report,
                           kernel_versions=kernel_versions,
                           kernel_reports=kernel_reports,
                           cur_kernel_report=cur_kernel_report,
                           graph_data=graph_data)


@app.route('/')
def index():
    """
    Defines the default reports and shows the overview.
    """

    global cur_kernel_report
    global cur_report

    try:
        if len(kernel_reports) <= 0:
            raise Exception("No kernel reports found!")

        default_kernel_report = list(kernel_reports.values())[0]
        default_report = list(default_kernel_report.reports.values())[0]

        cur_kernel_report = default_kernel_report
        cur_report = default_report
        return redirect(url_for("overview", kernel_version=cur_kernel_report.version, syscall_name=cur_report.syscall_name))
    except Exception as e:
        print("Error: " + str(e))
        return(str(e))


if __name__ == "__main__":
    init_kernel_reports()
    create_overview_kernel_graph()
    create_kernel_context_graph()
    # -----------

    # #create_mem_usage_graph()

    # app.run(host="130.83.40.251", port="5000")
    app.run()
