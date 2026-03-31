def can_build(env, platform):
    env.module_add_dependencies("lottie", ["svg"], True)
    return True


def configure(env):
    pass


def get_doc_classes():
    return ["ThorVGLottie"]


def get_doc_path():
    return "doc_classes"
