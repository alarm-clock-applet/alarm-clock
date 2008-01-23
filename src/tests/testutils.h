#ifndef TESTUTILS_H_
#define TESTUTILS_H_

#define ASSERT_EQUALS(v1,v2)			g_assert (v1 == v2)

#define ASSERT_STR_EQUALS(s1,s2)		g_assert (strcmp(s1,s2) == 0)

#define ASSERT_PROP_EQUALS(o,key,eq,type)	do {										\
												type val;								\
												g_object_get ((o), (key), &val, NULL);	\
												ASSERT_EQUALS(val,eq);					\
											} while (0)

#define ASSERT_PROP_STR_EQUALS(o,key,eq)	do {										\
												gchar *val;								\
												g_object_get ((o), (key), &val, NULL);	\
												ASSERT_STR_EQUALS (val, eq);			\
											} while (0)

#endif /*TESTUTILS_H_*/
