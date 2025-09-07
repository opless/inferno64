int
_tas(int *x)
{
#if __has_builtin(__atomic_test_and_set) || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 7)))
    return __atomic_test_and_set(x, __ATOMIC_ACQ_REL);
#else
#error __atomic_test_and_set does not exist
#endif
}