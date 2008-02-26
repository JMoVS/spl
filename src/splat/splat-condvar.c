#include <sys/zfs_context.h>
#include <sys/splat-ctl.h>

#define KZT_SUBSYSTEM_CONDVAR		0x0500
#define KZT_CONDVAR_NAME		"condvar"
#define KZT_CONDVAR_DESC		"Kernel Condition Variable Tests"

#define KZT_CONDVAR_TEST1_ID		0x0501
#define KZT_CONDVAR_TEST1_NAME		"signal1"
#define KZT_CONDVAR_TEST1_DESC		"Wake a single thread, cv_wait()/cv_signal()"

#define KZT_CONDVAR_TEST2_ID		0x0502
#define KZT_CONDVAR_TEST2_NAME		"broadcast1"
#define KZT_CONDVAR_TEST2_DESC		"Wake all threads, cv_wait()/cv_broadcast()"

#define KZT_CONDVAR_TEST3_ID		0x0503
#define KZT_CONDVAR_TEST3_NAME		"signal2"
#define KZT_CONDVAR_TEST3_DESC		"Wake a single thread, cv_wait_timeout()/cv_signal()"

#define KZT_CONDVAR_TEST4_ID		0x0504
#define KZT_CONDVAR_TEST4_NAME		"broadcast2"
#define KZT_CONDVAR_TEST4_DESC		"Wake all threads, cv_wait_timeout()/cv_broadcast()"

#define KZT_CONDVAR_TEST5_ID		0x0505
#define KZT_CONDVAR_TEST5_NAME		"timeout"
#define KZT_CONDVAR_TEST5_DESC		"Timeout thread, cv_wait_timeout()"

#define KZT_CONDVAR_TEST_MAGIC		0x115599DDUL
#define KZT_CONDVAR_TEST_NAME		"condvar_test"
#define KZT_CONDVAR_TEST_COUNT		8

typedef struct condvar_priv {
        unsigned long cv_magic;
        struct file *cv_file;
	kcondvar_t cv_condvar;
	kmutex_t cv_mtx;
} condvar_priv_t;

typedef struct condvar_thr {
	int ct_id;
	const char *ct_name;
	condvar_priv_t *ct_cvp;
	int ct_rc;
} condvar_thr_t;

int
kzt_condvar_test12_thread(void *arg)
{
	condvar_thr_t *ct = (condvar_thr_t *)arg;
	condvar_priv_t *cv = ct->ct_cvp;
	char name[16];

	ASSERT(cv->cv_magic == KZT_CONDVAR_TEST_MAGIC);
        snprintf(name, sizeof(name), "%s%d", KZT_CONDVAR_TEST_NAME, ct->ct_id);
	daemonize(name);

	mutex_enter(&cv->cv_mtx);
	kzt_vprint(cv->cv_file, ct->ct_name,
	           "%s thread sleeping with %d waiters\n",
		   name, atomic_read(&cv->cv_condvar.cv_waiters));
	cv_wait(&cv->cv_condvar, &cv->cv_mtx);
	kzt_vprint(cv->cv_file, ct->ct_name,
	           "%s thread woken %d waiters remain\n",
		   name, atomic_read(&cv->cv_condvar.cv_waiters));
	mutex_exit(&cv->cv_mtx);

	return 0;
}

static int
kzt_condvar_test1(struct file *file, void *arg)
{
	int i, count = 0, rc = 0;
	long pids[KZT_CONDVAR_TEST_COUNT];
	condvar_thr_t ct[KZT_CONDVAR_TEST_COUNT];
	condvar_priv_t cv;

	cv.cv_magic = KZT_CONDVAR_TEST_MAGIC;
	cv.cv_file = file;
	mutex_init(&cv.cv_mtx, KZT_CONDVAR_TEST_NAME, MUTEX_DEFAULT, NULL);
	cv_init(&cv.cv_condvar, KZT_CONDVAR_TEST_NAME, CV_DEFAULT, NULL);

	/* Create some threads, the exact number isn't important just as
	 * long as we know how many we managed to create and should expect. */
	for (i = 0; i < KZT_CONDVAR_TEST_COUNT; i++) {
		ct[i].ct_cvp = &cv;
		ct[i].ct_id = i;
		ct[i].ct_name = KZT_CONDVAR_TEST1_NAME;
		ct[i].ct_rc = 0;

		pids[i] = kernel_thread(kzt_condvar_test12_thread, &ct[i], 0);
		if (pids[i] >= 0)
			count++;
	}

	/* Wait until all threads are waiting on the condition variable */
	while (atomic_read(&cv.cv_condvar.cv_waiters) != count)
		schedule();

	/* Wake a single thread at a time, wait until it exits */
	for (i = 1; i <= count; i++) {
		cv_signal(&cv.cv_condvar);

		while (atomic_read(&cv.cv_condvar.cv_waiters) > (count - i))
			schedule();

		/* Correct behavior 1 thread woken */
		if (atomic_read(&cv.cv_condvar.cv_waiters) == (count - i))
			continue;

                kzt_vprint(file, KZT_CONDVAR_TEST1_NAME, "Attempted to "
			   "wake %d thread but work %d threads woke\n",
			   1, count - atomic_read(&cv.cv_condvar.cv_waiters));
		rc = -EINVAL;
		break;
	}

	if (!rc)
                kzt_vprint(file, KZT_CONDVAR_TEST1_NAME, "Correctly woke "
			   "%d sleeping threads %d at a time\n", count, 1);

	/* Wait until that last nutex is dropped */
	while (mutex_owner(&cv.cv_mtx))
		schedule();

	/* Wake everything for the failure case */
	cv_broadcast(&cv.cv_condvar);
	cv_destroy(&cv.cv_condvar);
	mutex_destroy(&cv.cv_mtx);

	return rc;
}

static int
kzt_condvar_test2(struct file *file, void *arg)
{
	int i, count = 0, rc = 0;
	long pids[KZT_CONDVAR_TEST_COUNT];
	condvar_thr_t ct[KZT_CONDVAR_TEST_COUNT];
	condvar_priv_t cv;

	cv.cv_magic = KZT_CONDVAR_TEST_MAGIC;
	cv.cv_file = file;
	mutex_init(&cv.cv_mtx, KZT_CONDVAR_TEST_NAME, MUTEX_DEFAULT, NULL);
	cv_init(&cv.cv_condvar, KZT_CONDVAR_TEST_NAME, CV_DEFAULT, NULL);

	/* Create some threads, the exact number isn't important just as
	 * long as we know how many we managed to create and should expect. */
	for (i = 0; i < KZT_CONDVAR_TEST_COUNT; i++) {
		ct[i].ct_cvp = &cv;
		ct[i].ct_id = i;
		ct[i].ct_name = KZT_CONDVAR_TEST2_NAME;
		ct[i].ct_rc = 0;

		pids[i] = kernel_thread(kzt_condvar_test12_thread, &ct[i], 0);
		if (pids[i] > 0)
			count++;
	}

	/* Wait until all threads are waiting on the condition variable */
	while (atomic_read(&cv.cv_condvar.cv_waiters) != count)
		schedule();

	/* Wake all threads waiting on the condition variable */
	cv_broadcast(&cv.cv_condvar);

	/* Wait until all threads have exited */
	while ((atomic_read(&cv.cv_condvar.cv_waiters) > 0) || mutex_owner(&cv.cv_mtx))
		schedule();

        kzt_vprint(file, KZT_CONDVAR_TEST2_NAME, "Correctly woke all "
			   "%d sleeping threads at once\n", count);

	/* Wake everything for the failure case */
	cv_destroy(&cv.cv_condvar);
	mutex_destroy(&cv.cv_mtx);

	return rc;
}

int
kzt_condvar_test34_thread(void *arg)
{
	condvar_thr_t *ct = (condvar_thr_t *)arg;
	condvar_priv_t *cv = ct->ct_cvp;
	char name[16];
	clock_t rc;

	ASSERT(cv->cv_magic == KZT_CONDVAR_TEST_MAGIC);
        snprintf(name, sizeof(name), "%s%d", KZT_CONDVAR_TEST_NAME, ct->ct_id);
	daemonize(name);

	mutex_enter(&cv->cv_mtx);
	kzt_vprint(cv->cv_file, ct->ct_name,
	           "%s thread sleeping with %d waiters\n",
		   name, atomic_read(&cv->cv_condvar.cv_waiters));

	/* Sleep no longer than 3 seconds, for this test we should
	 * actually never sleep that long without being woken up. */
	rc = cv_timedwait(&cv->cv_condvar, &cv->cv_mtx, lbolt + HZ * 3);
	if (rc == -1) {
		ct->ct_rc = -ETIMEDOUT;
		kzt_vprint(cv->cv_file, ct->ct_name, "%s thread timed out, "
		           "should have been woken\n", name);
	} else {
		kzt_vprint(cv->cv_file, ct->ct_name,
		           "%s thread woken %d waiters remain\n",
			   name, atomic_read(&cv->cv_condvar.cv_waiters));
	}

	mutex_exit(&cv->cv_mtx);

	return 0;
}

static int
kzt_condvar_test3(struct file *file, void *arg)
{
	int i, count = 0, rc = 0;
	long pids[KZT_CONDVAR_TEST_COUNT];
	condvar_thr_t ct[KZT_CONDVAR_TEST_COUNT];
	condvar_priv_t cv;

	cv.cv_magic = KZT_CONDVAR_TEST_MAGIC;
	cv.cv_file = file;
	mutex_init(&cv.cv_mtx, KZT_CONDVAR_TEST_NAME, MUTEX_DEFAULT, NULL);
	cv_init(&cv.cv_condvar, KZT_CONDVAR_TEST_NAME, CV_DEFAULT, NULL);

	/* Create some threads, the exact number isn't important just as
	 * long as we know how many we managed to create and should expect. */
	for (i = 0; i < KZT_CONDVAR_TEST_COUNT; i++) {
		ct[i].ct_cvp = &cv;
		ct[i].ct_id = i;
		ct[i].ct_name = KZT_CONDVAR_TEST3_NAME;
		ct[i].ct_rc = 0;

		pids[i] = kernel_thread(kzt_condvar_test34_thread, &ct[i], 0);
		if (pids[i] >= 0)
			count++;
	}

	/* Wait until all threads are waiting on the condition variable */
	while (atomic_read(&cv.cv_condvar.cv_waiters) != count)
		schedule();

	/* Wake a single thread at a time, wait until it exits */
	for (i = 1; i <= count; i++) {
		cv_signal(&cv.cv_condvar);

		while (atomic_read(&cv.cv_condvar.cv_waiters) > (count - i))
			schedule();

		/* Correct behavior 1 thread woken */
		if (atomic_read(&cv.cv_condvar.cv_waiters) == (count - i))
			continue;

                kzt_vprint(file, KZT_CONDVAR_TEST3_NAME, "Attempted to "
			   "wake %d thread but work %d threads woke\n",
			   1, count - atomic_read(&cv.cv_condvar.cv_waiters));
		rc = -EINVAL;
		break;
	}

	/* Validate no waiting thread timed out early */
	for (i = 0; i < count; i++)
		if (ct[i].ct_rc)
			rc = ct[i].ct_rc;

	if (!rc)
                kzt_vprint(file, KZT_CONDVAR_TEST3_NAME, "Correctly woke "
			   "%d sleeping threads %d at a time\n", count, 1);

	/* Wait until that last nutex is dropped */
	while (mutex_owner(&cv.cv_mtx))
		schedule();

	/* Wake everything for the failure case */
	cv_broadcast(&cv.cv_condvar);
	cv_destroy(&cv.cv_condvar);
	mutex_destroy(&cv.cv_mtx);

	return rc;
}

static int
kzt_condvar_test4(struct file *file, void *arg)
{
	int i, count = 0, rc = 0;
	long pids[KZT_CONDVAR_TEST_COUNT];
	condvar_thr_t ct[KZT_CONDVAR_TEST_COUNT];
	condvar_priv_t cv;

	cv.cv_magic = KZT_CONDVAR_TEST_MAGIC;
	cv.cv_file = file;
	mutex_init(&cv.cv_mtx, KZT_CONDVAR_TEST_NAME, MUTEX_DEFAULT, NULL);
	cv_init(&cv.cv_condvar, KZT_CONDVAR_TEST_NAME, CV_DEFAULT, NULL);

	/* Create some threads, the exact number isn't important just as
	 * long as we know how many we managed to create and should expect. */
	for (i = 0; i < KZT_CONDVAR_TEST_COUNT; i++) {
		ct[i].ct_cvp = &cv;
		ct[i].ct_id = i;
		ct[i].ct_name = KZT_CONDVAR_TEST3_NAME;
		ct[i].ct_rc = 0;

		pids[i] = kernel_thread(kzt_condvar_test34_thread, &ct[i], 0);
		if (pids[i] >= 0)
			count++;
	}

	/* Wait until all threads are waiting on the condition variable */
	while (atomic_read(&cv.cv_condvar.cv_waiters) != count)
		schedule();

	/* Wake a single thread at a time, wait until it exits */
	for (i = 1; i <= count; i++) {
		cv_signal(&cv.cv_condvar);

		while (atomic_read(&cv.cv_condvar.cv_waiters) > (count - i))
			schedule();

		/* Correct behavior 1 thread woken */
		if (atomic_read(&cv.cv_condvar.cv_waiters) == (count - i))
			continue;

                kzt_vprint(file, KZT_CONDVAR_TEST3_NAME, "Attempted to "
			   "wake %d thread but work %d threads woke\n",
			   1, count - atomic_read(&cv.cv_condvar.cv_waiters));
		rc = -EINVAL;
		break;
	}

	/* Validate no waiting thread timed out early */
	for (i = 0; i < count; i++)
		if (ct[i].ct_rc)
			rc = ct[i].ct_rc;

	if (!rc)
                kzt_vprint(file, KZT_CONDVAR_TEST3_NAME, "Correctly woke "
			   "%d sleeping threads %d at a time\n", count, 1);

	/* Wait until that last nutex is dropped */
	while (mutex_owner(&cv.cv_mtx))
		schedule();

	/* Wake everything for the failure case */
	cv_broadcast(&cv.cv_condvar);
	cv_destroy(&cv.cv_condvar);
	mutex_destroy(&cv.cv_mtx);

	return rc;
}

static int
kzt_condvar_test5(struct file *file, void *arg)
{
        kcondvar_t condvar;
        kmutex_t mtx;
	clock_t time_left, time_before, time_after, time_delta;
	int64_t whole_delta;
	int32_t remain_delta;
	int rc = 0;

	mutex_init(&mtx, KZT_CONDVAR_TEST_NAME, MUTEX_DEFAULT, NULL);
	cv_init(&condvar, KZT_CONDVAR_TEST_NAME, CV_DEFAULT, NULL);

        kzt_vprint(file, KZT_CONDVAR_TEST5_NAME, "Thread going to sleep for "
	           "%d second and expecting to be woken by timeout\n", 1);

	/* Allow a 1 second timeout, plenty long to validate correctness. */
	time_before = lbolt;
	mutex_enter(&mtx);
	time_left = cv_timedwait(&condvar, &mtx, lbolt + HZ);
	mutex_exit(&mtx);
	time_after = lbolt;
	time_delta = time_after - time_before; /* XXX - Handle jiffie wrap */
	whole_delta  = time_delta;
	remain_delta = do_div(whole_delta, HZ);

	if (time_left == -1) {
		if (time_delta >= HZ) {
	        	kzt_vprint(file, KZT_CONDVAR_TEST5_NAME,
			           "Thread correctly timed out and was asleep "
			           "for %d.%d seconds (%d second min)\n",
			           (int)whole_delta, remain_delta, 1);
		} else {
	        	kzt_vprint(file, KZT_CONDVAR_TEST5_NAME,
			           "Thread correctly timed out but was only "
			           "asleep for %d.%d seconds (%d second "
			           "min)\n", (int)whole_delta, remain_delta, 1);
			rc = -ETIMEDOUT;
		}
	} else {
        	kzt_vprint(file, KZT_CONDVAR_TEST5_NAME,
		           "Thread exited after only %d.%d seconds, it "
		           "did not hit the %d second timeout\n",
		           (int)whole_delta, remain_delta, 1);
		rc = -ETIMEDOUT;
	}

	cv_destroy(&condvar);
	mutex_destroy(&mtx);

	return rc;
}

kzt_subsystem_t *
kzt_condvar_init(void)
{
        kzt_subsystem_t *sub;

        sub = kmalloc(sizeof(*sub), GFP_KERNEL);
        if (sub == NULL)
                return NULL;

        memset(sub, 0, sizeof(*sub));
        strncpy(sub->desc.name, KZT_CONDVAR_NAME, KZT_NAME_SIZE);
        strncpy(sub->desc.desc, KZT_CONDVAR_DESC, KZT_DESC_SIZE);
        INIT_LIST_HEAD(&sub->subsystem_list);
        INIT_LIST_HEAD(&sub->test_list);
        spin_lock_init(&sub->test_lock);
        sub->desc.id = KZT_SUBSYSTEM_CONDVAR;

        KZT_TEST_INIT(sub, KZT_CONDVAR_TEST1_NAME, KZT_CONDVAR_TEST1_DESC,
                      KZT_CONDVAR_TEST1_ID, kzt_condvar_test1);
        KZT_TEST_INIT(sub, KZT_CONDVAR_TEST2_NAME, KZT_CONDVAR_TEST2_DESC,
                      KZT_CONDVAR_TEST2_ID, kzt_condvar_test2);
        KZT_TEST_INIT(sub, KZT_CONDVAR_TEST3_NAME, KZT_CONDVAR_TEST3_DESC,
                      KZT_CONDVAR_TEST3_ID, kzt_condvar_test3);
        KZT_TEST_INIT(sub, KZT_CONDVAR_TEST4_NAME, KZT_CONDVAR_TEST4_DESC,
                      KZT_CONDVAR_TEST4_ID, kzt_condvar_test4);
        KZT_TEST_INIT(sub, KZT_CONDVAR_TEST5_NAME, KZT_CONDVAR_TEST5_DESC,
                      KZT_CONDVAR_TEST5_ID, kzt_condvar_test5);

        return sub;
}

void
kzt_condvar_fini(kzt_subsystem_t *sub)
{
        ASSERT(sub);
        KZT_TEST_FINI(sub, KZT_CONDVAR_TEST5_ID);
        KZT_TEST_FINI(sub, KZT_CONDVAR_TEST4_ID);
        KZT_TEST_FINI(sub, KZT_CONDVAR_TEST3_ID);
        KZT_TEST_FINI(sub, KZT_CONDVAR_TEST2_ID);
        KZT_TEST_FINI(sub, KZT_CONDVAR_TEST1_ID);

        kfree(sub);
}

int
kzt_condvar_id(void) {
        return KZT_SUBSYSTEM_CONDVAR;
}