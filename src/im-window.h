#ifndef IM_WINDOW_H
#define IM_WINDOW_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define IM_TYPE_WINDOW	        (im_window_get_type ())
#define IM_WINDOW(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), IM_TYPE_WINDOW, ImWindow))
#define IM_WINDOW_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), IM_TYPE_WINDOW, ImWindowClass))
#define IM_IS_WINDOW(o)	        (G_TYPE_CHECK_INSTANCE_TYPE ((o), IM_TYPE_WINDOW))
#define IM_IS_WINDOW_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), IM_TYPE_WINDOW))
#define IM_WINDOW_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), IM_TYPE_WINDOW, ImWindowClass))

typedef struct _ImWindowClass		ImWindowClass;
typedef struct _ImWindow		ImWindow;
typedef struct _ImWindowPrivate	        ImWindowPrivate;

struct _ImWindow
{
	GtkApplicationWindow parent;

	/*< private >*/
	ImWindowPrivate *priv;
};

struct _ImWindowClass
{
	GtkApplicationWindowClass parent_class;
};

GType		  im_window_get_type		  (void);

GtkWidget	 *im_window_new		          (GtkApplication *window);

G_END_DECLS

#endif
