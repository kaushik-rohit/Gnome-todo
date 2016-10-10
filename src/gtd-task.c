/* gtd-task.c
 *
 * Copyright (C) 2015 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gtd-task.h"
#include "gtd-task-list.h"

#include <glib/gi18n.h>
#include <libecal/libecal.h>
#include <libical/icaltime.h>
#include <libical/icaltimezone.h>

/**
 * SECTION:gtd-task
 * @short_description: a task
 * @title:GtdTask
 * @stability:Unstable
 * @see_also:#GtdTaskList
 *
 * A #GtdTask is an object that represents a task. All #GtdTasks
 * must be inside a #GtdTaskList.
 */

typedef struct
{
  gchar           *description;
  GtdTaskList     *list;
  ECalComponent   *component;
  GtdTask         *parent;
  GList           *subtasks;
} GtdTaskPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GtdTask, gtd_task, GTD_TYPE_OBJECT)

enum
{
  PROP_0,
  PROP_COMPLETE,
  PROP_COMPONENT,
  PROP_DESCRIPTION,
  PROP_CREATION_DATE,
  PROP_DUE_DATE,
  PROP_LIST,
  PROP_PARENT,
  PROP_PRIORITY,
  PROP_TITLE,
  LAST_PROP
};

enum
{
  SUBTASK_ADDED,
  SUBTASK_REMOVED,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0, };

static GDateTime*
gtd_task__convert_icaltime (const icaltimetype *date)
{
  GDateTime *dt;

  if (!date)
    return NULL;

  dt = g_date_time_new_utc (date->year,
                            date->month,
                            date->day,
                            date->is_date ? 0 : date->hour,
                            date->is_date ? 0 : date->minute,
                            date->is_date ? 0 : date->second);

  return dt;
}

static void
real_add_subtask (GtdTask *self,
                  GtdTask *subtask)
{
  GtdTaskPrivate *priv, *subtask_priv;
  ECalComponentId *id;
  ECalComponent *comp;
  icalcomponent *ical_comp;
  icalproperty *property;

  priv = gtd_task_get_instance_private (self);

  if (g_list_find (priv->subtasks, subtask))
    return;

  id = e_cal_component_get_id (priv->component);
  subtask_priv = gtd_task_get_instance_private (subtask);
  comp = subtask_priv->component;

  ical_comp = e_cal_component_get_icalcomponent (comp);
  property = icalcomponent_get_first_property (ical_comp, ICAL_RELATEDTO_PROPERTY);

  if (property)
    icalproperty_set_relatedto (property, id->uid);
  else
    icalcomponent_add_property (ical_comp, icalproperty_new_relatedto (id->uid));

  /* Add to this task's list of subtasks */
  priv->subtasks = g_list_prepend (priv->subtasks, subtask);

  /* Update the subtask's parent property */
  subtask_priv->parent = self;
  g_object_notify (G_OBJECT (subtask), "parent");

  e_cal_component_free_id (id);
}

static void
real_remove_subtask (GtdTask *self,
                     GtdTask *subtask)
{
  GtdTaskPrivate *priv, *subtask_priv;
  icalcomponent *ical_comp;
  icalproperty *property;

  priv = gtd_task_get_instance_private (self);

  if (!g_list_find (priv->subtasks, subtask))
    return;

  subtask_priv = gtd_task_get_instance_private (subtask);

  /* Remove the parent link from the subtask's component */
  ical_comp = e_cal_component_get_icalcomponent (subtask_priv->component);
  property = icalcomponent_get_first_property (ical_comp, ICAL_RELATEDTO_PROPERTY);

  if (!property)
    return;

  icalcomponent_remove_property (ical_comp, property);

  /* Add to this task's list of subtasks */
  priv->subtasks = g_list_remove (priv->subtasks, subtask);

  /* Update the subtask's parent property */
  subtask_priv->parent = NULL;
  g_object_notify (G_OBJECT (subtask), "parent");
}

static void
gtd_task_finalize (GObject *object)
{
  GtdTask *self = (GtdTask*) object;
  GtdTaskPrivate *priv = gtd_task_get_instance_private (self);

  g_free (priv->description);
  g_object_unref (priv->component);

  G_OBJECT_CLASS (gtd_task_parent_class)->finalize (object);
}

static const gchar*
gtd_task__get_uid (GtdObject *object)
{
  GtdTaskPrivate *priv = gtd_task_get_instance_private (GTD_TASK (object));
  const gchar *uid;

  g_return_val_if_fail (GTD_IS_TASK (object), NULL);

  if (priv->component)
    e_cal_component_get_uid (priv->component, &uid);
  else
    uid = NULL;

  return uid;
}

static void
gtd_task__set_uid (GtdObject   *object,
                   const gchar *uid)
{
  GtdTaskPrivate *priv = gtd_task_get_instance_private (GTD_TASK (object));
  const gchar *current_uid;

  g_return_if_fail (GTD_IS_TASK (object));

  if (!priv->component)
    return;

  e_cal_component_get_uid (priv->component, &current_uid);

  if (g_strcmp0 (current_uid, uid) != 0)
    {
      e_cal_component_set_uid (priv->component, uid);

      g_object_notify (G_OBJECT (object), "uid");
    }
}

static void
gtd_task_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  GtdTask *self = GTD_TASK (object);
  GtdTaskPrivate *priv = gtd_task_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_COMPLETE:
      g_value_set_boolean (value, gtd_task_get_complete (self));
      break;

    case PROP_COMPONENT:
      g_value_set_object (value, priv->component);
      break;

    case PROP_CREATION_DATE:
      g_value_set_boxed (value, gtd_task_get_creation_date (self));
      break;

    case PROP_DESCRIPTION:
      g_value_set_string (value, gtd_task_get_description (self));
      break;

    case PROP_DUE_DATE:
      g_value_set_boxed (value, gtd_task_get_due_date (self));
      break;

    case PROP_LIST:
      g_value_set_object (value, priv->list);
      break;

    case PROP_PARENT:
      g_value_set_object (value, priv->parent);
      break;

    case PROP_PRIORITY:
      g_value_set_int (value, gtd_task_get_priority (self));
      break;

    case PROP_TITLE:
      g_value_set_string (value, gtd_task_get_title (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtd_task_set_property (GObject      *object,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  GtdTask *self = GTD_TASK (object);
  GtdTaskPrivate *priv = gtd_task_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_COMPLETE:
      gtd_task_set_complete (self, g_value_get_boolean (value));
      break;

    case PROP_COMPONENT:
      priv->component = g_value_get_object (value);

      if (!priv->component)
        {
          priv->component = e_cal_component_new ();
          e_cal_component_set_new_vtype (priv->component, E_CAL_COMPONENT_TODO);
        }
      else
        {
          g_object_ref (priv->component);
        }

      break;

    case PROP_DESCRIPTION:
      gtd_task_set_description (self, g_value_get_string (value));
      break;

    case PROP_DUE_DATE:
      gtd_task_set_due_date (self, g_value_get_boxed (value));
      break;

    case PROP_LIST:
      gtd_task_set_list (self, g_value_get_object (value));
      break;

    case PROP_PRIORITY:
      gtd_task_set_priority (self, g_value_get_int (value));
      break;

    case PROP_TITLE:
      gtd_task_set_title (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtd_task_class_init (GtdTaskClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtdObjectClass *obj_class = GTD_OBJECT_CLASS (klass);

  klass->subtask_added = real_add_subtask;
  klass->subtask_removed = real_remove_subtask;

  object_class->finalize = gtd_task_finalize;
  object_class->get_property = gtd_task_get_property;
  object_class->set_property = gtd_task_set_property;

  obj_class->get_uid = gtd_task__get_uid;
  obj_class->set_uid = gtd_task__set_uid;

  /**
   * GtdTask::complete:
   *
   * @TRUE if the task is marked as complete or @FALSE otherwise. Usually
   * represented by a checkbox at user interfaces.
   */
  g_object_class_install_property (
        object_class,
        PROP_COMPLETE,
        g_param_spec_boolean ("complete",
                              "Whether the task is completed or not",
                              "Whether the task is marked as completed by the user",
                              FALSE,
                              G_PARAM_READWRITE));

  /**
   * GtdTask::component:
   *
   * The #ECalComponent of the task.
   */
  g_object_class_install_property (
        object_class,
        PROP_COMPONENT,
        g_param_spec_object ("component",
                              "Component of the task",
                              "The #ECalComponent this task handles.",
                              E_TYPE_CAL_COMPONENT,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  /**
   * GtdTask::creation-date:
   *
   * The @GDateTime that represents the time in which the task was created.
   */
  g_object_class_install_property (
        object_class,
        PROP_CREATION_DATE,
        g_param_spec_boxed ("creation-date",
                            "Creation date of the task",
                            "The day the task was created.",
                            G_TYPE_DATE_TIME,
                            G_PARAM_READABLE));

  /**
   * GtdTask::description:
   *
   * Description of the task.
   */
  g_object_class_install_property (
        object_class,
        PROP_DESCRIPTION,
        g_param_spec_string ("description",
                             "Description of the task",
                             "Optional string describing the task",
                             NULL,
                             G_PARAM_READWRITE));

  /**
   * GtdTask::due-date:
   *
   * The @GDateTime that represents the time in which the task should
   * be completed before.
   */
  g_object_class_install_property (
        object_class,
        PROP_DUE_DATE,
        g_param_spec_boxed ("due-date",
                            "End date of the task",
                            "The day the task is supposed to be completed",
                            G_TYPE_DATE_TIME,
                            G_PARAM_READWRITE));

  /**
   * GtdTask::list:
   *
   * The @GtdTaskList that contains this task.
   */
  g_object_class_install_property (
        object_class,
        PROP_LIST,
        g_param_spec_object ("list",
                             "List of the task",
                             "The list that owns this task",
                             GTD_TYPE_TASK_LIST,
                             G_PARAM_READWRITE));

  /**
   * GtdTask::parent:
   *
   * The parent of the task.
   */
  g_object_class_install_property (
        object_class,
        PROP_PARENT,
        g_param_spec_object ("parent",
                              "Parent of the task",
                              "The GtdTask that is parent of this task.",
                              GTD_TYPE_TASK,
                              G_PARAM_READABLE));

  /**
   * GtdTask::priority:
   *
   * Priority of the task, 0 if not set.
   */
  g_object_class_install_property (
        object_class,
        PROP_PRIORITY,
        g_param_spec_int ("priority",
                          "Priority of the task",
                          "The priority of the task. 0 means no priority set, and tasks will be sorted alphabetically.",
                          0,
                          G_MAXINT,
                          0,
                          G_PARAM_READWRITE));

  /**
   * GtdTask::title:
   *
   * The title of the task, usually the task name.
   */
  g_object_class_install_property (
        object_class,
        PROP_TITLE,
        g_param_spec_string ("title",
                             "Title of the task",
                             "The title of the task",
                             NULL,
                             G_PARAM_READWRITE));

  /* Signals */
  /**
   * GtdTask:subtask-added:
   *
   * Emited when a subtask is added to @self.
   */
  signals[SUBTASK_ADDED] = g_signal_new ("subtask-added",
                                         GTD_TYPE_TASK,
                                         G_SIGNAL_RUN_LAST,
                                         G_STRUCT_OFFSET (GtdTaskClass, subtask_added),
                                         NULL,
                                         NULL,
                                         NULL,
                                         G_TYPE_NONE,
                                         1,
                                         GTD_TYPE_TASK);

  /**
   * GtdTask:subtask-added:
   *
   * Emited when a subtask is added to @self.
   */
  signals[SUBTASK_REMOVED] = g_signal_new ("subtask-removed",
                                           GTD_TYPE_TASK,
                                           G_SIGNAL_RUN_LAST,
                                           G_STRUCT_OFFSET (GtdTaskClass, subtask_removed),
                                           NULL,
                                           NULL,
                                           NULL,
                                           G_TYPE_NONE,
                                           1,
                                           GTD_TYPE_TASK);
}

static void
gtd_task_init (GtdTask *self)
{
  ;
}

/**
 * gtd_task_new:
 * @component: (nullable): a #ECalComponent
 *
 * Creates a new #GtdTask
 *
 * Returns: (transfer full): a #GtdTask
 */
GtdTask *
gtd_task_new (ECalComponent *component)
{
  const gchar *uid;

  if (component)
    e_cal_component_get_uid (component, &uid);
  else
    uid = NULL;

  return g_object_new (GTD_TYPE_TASK,
                       "component", component,
                       NULL);
}

/**
 * gtd_task_get_complete:
 * @task: a #GtdTask
 *
 * Retrieves whether the task is complete or not.
 *
 * Returns: %TRUE if the task is complete, %FALSE otherwise
 */
gboolean
gtd_task_get_complete (GtdTask *task)
{
  GtdTaskPrivate *priv;
  icaltimetype *dt;
  gboolean completed;

  g_return_val_if_fail (GTD_IS_TASK (task), FALSE);

  priv = gtd_task_get_instance_private (task);

  e_cal_component_get_completed (priv->component, &dt);
  completed = (dt != NULL);

  if (dt)
    e_cal_component_free_icaltimetype (dt);

  return completed;
}

/**
 * gtd_task_get_component:
 * @task: a #GtdTask
 *
 * Retrieves the internal #ECalComponent of @task.
 *
 * Returns: (transfer none): a #ECalComponent
 */
ECalComponent*
gtd_task_get_component (GtdTask *task)
{
  GtdTaskPrivate *priv;

  g_return_val_if_fail (GTD_IS_TASK (task), NULL);

  priv = gtd_task_get_instance_private (task);

  return priv->component;
}

/**
 * gtd_task_set_complete:
 * @task: a #GtdTask
 * @complete: the new value
 *
 * Updates the complete state of @task.
 */
void
gtd_task_set_complete (GtdTask  *task,
                       gboolean  complete)
{
  GtdTaskPrivate *priv;

  g_assert (GTD_IS_TASK (task));

  priv = gtd_task_get_instance_private (task);

  if (gtd_task_get_complete (task) != complete)
    {
      icaltimetype *dt;
      icalproperty_status status;
      gint percent;

      if (complete)
        {
          GDateTime *now = g_date_time_new_now_utc ();

          percent = 100;
          status = ICAL_STATUS_COMPLETED;

          dt = g_new0 (icaltimetype, 1);
          dt->year = g_date_time_get_year (now);
          dt->month = g_date_time_get_month (now);
          dt->day = g_date_time_get_day_of_month (now);
          dt->hour = g_date_time_get_hour (now);
          dt->minute = g_date_time_get_minute (now);
          dt->second = g_date_time_get_seconds (now);
          dt->is_date = 0;
          dt->is_utc = 1;

          /* convert timezone
           *
           * FIXME: This does not do anything until we have an ical
           * timezone associated with the task
           */
          icaltimezone_convert_time (dt,
                                     NULL,
                                     icaltimezone_get_utc_timezone ());

        }
      else
        {
          dt = NULL;
          percent = 0;
          status = ICAL_STATUS_NEEDSACTION;
        }

      e_cal_component_set_percent_as_int (priv->component, percent);
      e_cal_component_set_status (priv->component, status);
      e_cal_component_set_completed (priv->component, dt);

      if (dt)
        e_cal_component_free_icaltimetype (dt);

      g_object_notify (G_OBJECT (task), "complete");
    }
}

/**
 * gtd_task_get_creation_date:
 * @task: a #GtdTask
 *
 * Returns the #GDateTime that represents the task's creation date.
 * The value is referenced for thread safety. Returns %NULL if
 * no date is set.
 *
 * Returns: (transfer full): the internal #GDateTime referenced
 * for thread safety, or %NULL. Unreference it after use.
 */
GDateTime*
gtd_task_get_creation_date (GtdTask *task)
{
  GtdTaskPrivate *priv;
  icaltimetype *idt;
  GDateTime *dt;

  g_return_val_if_fail (GTD_IS_TASK (task), NULL);
  priv = gtd_task_get_instance_private (task);

  idt = NULL;
  dt = NULL;

  e_cal_component_get_created (priv->component, &idt);

  if (idt)
    dt = gtd_task__convert_icaltime (idt);

  g_clear_pointer (&idt, e_cal_component_free_icaltimetype);

  return dt;
}

/**
 * gtd_task_get_description:
 * @task: a #GtdTask
 *
 * Retrieves the description of the task.
 *
 * Returns: (transfer none): the description of @task
 */
const gchar*
gtd_task_get_description (GtdTask *task)
{
  GtdTaskPrivate *priv;
  GSList *text_list;
  GSList *l;
  gchar *desc = NULL;

  g_return_val_if_fail (GTD_IS_TASK (task), NULL);

  priv = gtd_task_get_instance_private (task);

  /* concatenates the multiple descriptions a task may have */
  e_cal_component_get_description_list (priv->component, &text_list);
  for (l = text_list; l != NULL; l = l->next)
    {
      if (l->data != NULL)
        {
          ECalComponentText *text;
          gchar *carrier;
          text = l->data;

          if (desc != NULL)
            {
              carrier = g_strconcat (desc,
                                     "\n",
                                     text->value,
                                     NULL);
              g_free (desc);
              desc = carrier;
            }
          else
            {
              desc = g_strdup (text->value);
            }
        }
    }

  if (g_strcmp0 (priv->description, desc) != 0)
    {
      g_clear_pointer (&priv->description, g_free);

      priv->description = g_strdup (desc);
    }

  g_free (desc);
  e_cal_component_free_text_list (text_list);

  return priv->description ? priv->description : "";
}

/**
 * gtd_task_set_description:
 * @task: a #GtdTask
 * @description: (nullable): the new description, or %NULL
 *
 * Updates the description of @task. The string is not stripped off of
 * spaces to preserve user data.
 */
void
gtd_task_set_description (GtdTask     *task,
                          const gchar *description)
{
  GtdTaskPrivate *priv;

  g_assert (GTD_IS_TASK (task));
  g_assert (g_utf8_validate (description, -1, NULL));

  priv = gtd_task_get_instance_private (task);

  if (g_strcmp0 (priv->description, description) != 0)
    {
      GSList note;
      ECalComponentText text;

      g_clear_pointer (&priv->description, g_free);

      priv->description = g_strdup (description);

      text.value = priv->description;
      text.altrep = NULL;

      note.data = &text;
      note.next = NULL;

      e_cal_component_set_description_list (priv->component, &note);

      g_object_notify (G_OBJECT (task), "description");
    }
}

/**
 * gtd_task_get_due_date:
 * @task: a #GtdTask
 *
 * Returns the #GDateTime that represents the task's due date.
 * The value is referenced for thread safety. Returns %NULL if
 * no date is set.
 *
 * Returns: (transfer full): the internal #GDateTime referenced
 * for thread safety, or %NULL. Unreference it after use.
 */
GDateTime*
gtd_task_get_due_date (GtdTask *task)
{
  ECalComponentDateTime comp_dt;
  GtdTaskPrivate *priv;

  g_return_val_if_fail (GTD_IS_TASK (task), NULL);

  priv = gtd_task_get_instance_private (task);

  e_cal_component_get_due (priv->component, &comp_dt);

  return gtd_task__convert_icaltime (comp_dt.value);
}

/**
 * gtd_task_set_due_date:
 * @task: a #GtdTask
 * @dt: (nullable): a #GDateTime
 *
 * Updates the internal @GtdTask::due-date property.
 */
void
gtd_task_set_due_date (GtdTask   *task,
                       GDateTime *dt)
{
  GtdTaskPrivate *priv;
  GDateTime *current_dt;

  g_assert (GTD_IS_TASK (task));

  priv = gtd_task_get_instance_private (task);

  current_dt = gtd_task_get_due_date (task);

  if (dt != current_dt)
    {
      ECalComponentDateTime comp_dt;
      icaltimetype *idt;
      gboolean changed = FALSE;

      comp_dt.value = NULL;
      comp_dt.tzid = NULL;
      idt = NULL;

      if (!current_dt ||
          (current_dt &&
           dt &&
           g_date_time_compare (current_dt, dt) != 0))
        {
          idt = g_new0 (icaltimetype, 1);

          g_date_time_ref (dt);

          /* Copy the given dt */
          idt->year = g_date_time_get_year (dt);
          idt->month = g_date_time_get_month (dt);
          idt->day = g_date_time_get_day_of_month (dt);
          idt->hour = g_date_time_get_hour (dt);
          idt->minute = g_date_time_get_minute (dt);
          idt->second = g_date_time_get_seconds (dt);
          idt->is_date = (idt->hour == 0 &&
                          idt->minute == 0 &&
                          idt->second == 0);

          comp_dt.tzid = g_strdup ("UTC");

          comp_dt.value = idt;

          e_cal_component_set_due (priv->component, &comp_dt);

          e_cal_component_free_datetime (&comp_dt);

          g_date_time_unref (dt);

          changed = TRUE;
        }
      else if (!dt)
        {
          idt = NULL;
          comp_dt.tzid = NULL;

          e_cal_component_set_due (priv->component, NULL);

          changed = TRUE;
        }

      if (changed)
        g_object_notify (G_OBJECT (task), "due-date");
    }

  g_clear_pointer (&current_dt, g_date_time_unref);
}

/**
 * gtd_task_get_list:
 *
 * Returns a weak reference to the #GtdTaskList that
 * owns the given @task.
 *
 * Returns: (transfer none): a weak reference to the
 * #GtdTaskList that owns @task. Do not free after
 * usage.
 */
GtdTaskList*
gtd_task_get_list (GtdTask *task)
{
  GtdTaskPrivate *priv;

  g_return_val_if_fail (GTD_IS_TASK (task), NULL);

  priv = gtd_task_get_instance_private (task);

  return priv->list;
}

/**
 * gtd_task_set_list:
 * @task: a #GtdTask
 * @list: (nullable): a #GtdTaskList
 *
 * Sets the parent #GtdTaskList of @task.
 */
void
gtd_task_set_list (GtdTask     *task,
                   GtdTaskList *list)
{
  GtdTaskPrivate *priv;

  g_assert (GTD_IS_TASK (task));
  g_assert (GTD_IS_TASK_LIST (list));

  priv = gtd_task_get_instance_private (task);

  if (priv->list != list)
    {
      priv->list = list;
      g_object_notify (G_OBJECT (task), "list");
    }
}

/**
 * gtd_task_get_priority:
 * @task: a #GtdTask
 *
 * Returns the priority of @task inside the parent #GtdTaskList,
 * or -1 if not set.
 *
 * Returns: the priority of the task, or 0
 */
gint
gtd_task_get_priority (GtdTask *task)
{
  GtdTaskPrivate *priv;
  gint *priority = NULL;
  gint p;

  g_assert (GTD_IS_TASK (task));

  priv = gtd_task_get_instance_private (task);

  e_cal_component_get_priority (priv->component, &priority);

  if (!priority)
    return -1;

  p = *priority;

  g_free (priority);

  return p;
}

/**
 * gtd_task_set_priority:
 * @task: a #GtdTask
 * @priority: the priority of @task, or -1
 *
 * Sets the @task priority inside the parent #GtdTaskList. It
 * is up to the interface to handle two or more #GtdTask with
 * the same priority value.
 */
void
gtd_task_set_priority (GtdTask *task,
                       gint     priority)
{
  GtdTaskPrivate *priv;
  gint current;

  g_assert (GTD_IS_TASK (task));
  g_assert (priority >= -1);

  priv = gtd_task_get_instance_private (task);
  current = gtd_task_get_priority (task);

  if (priority != current)
    {
      e_cal_component_set_priority (priv->component, &priority);
      g_object_notify (G_OBJECT (task), "priority");
    }
}

/**
 * gtd_task_get_title:
 * @task: a #GtdTask
 *
 * Retrieves the title of the task, or %NULL.
 *
 * Returns: (transfer none): the title of @task, or %NULL
 */
const gchar*
gtd_task_get_title (GtdTask *task)
{
  GtdTaskPrivate *priv;
  ECalComponentText summary;

  g_return_val_if_fail (GTD_IS_TASK (task), NULL);

  priv = gtd_task_get_instance_private (task);

  e_cal_component_get_summary (priv->component, &summary);

  return summary.value ? summary.value : "";
}

/**
 * gtd_task_set_title:
 * @task: a #GtdTask
 * @title: (nullable): the new title, or %NULL
 *
 * Updates the title of @task. The string is stripped off of
 * leading spaces.
 */
void
gtd_task_set_title (GtdTask     *task,
                    const gchar *title)
{
  GtdTaskPrivate *priv;
  ECalComponentText summary;

  g_return_if_fail (GTD_IS_TASK (task));
  g_return_if_fail (g_utf8_validate (title, -1, NULL));

  priv = gtd_task_get_instance_private (task);

  e_cal_component_get_summary (priv->component, &summary);

  if (g_strcmp0 (summary.value, title) != 0)
    {
      ECalComponentText new_summary;

      new_summary.value = title;
      new_summary.altrep = NULL;

      e_cal_component_set_summary (priv->component, &new_summary);

      g_object_notify (G_OBJECT (task), "title");
    }
}

/**
 * gtd_task_abort:
 * @task: a #GtdTask
 *
 * Cancels any editing made on @task after the latest
 * call of @gtd_task_save.
 */
void
gtd_task_abort (GtdTask *task)
{
  GtdTaskPrivate *priv;

  g_return_if_fail (GTD_IS_TASK (task));

  priv = gtd_task_get_instance_private (task);

  e_cal_component_abort_sequence (priv->component);
}

/**
 * gtd_task_save:
 * @task: a #GtdTask
 *
 * Save any changes made on @task.
 */
void
gtd_task_save (GtdTask *task)
{
  GtdTaskPrivate *priv;

  g_return_if_fail (GTD_IS_TASK (task));

  priv = gtd_task_get_instance_private (task);

  e_cal_component_commit_sequence (priv->component);
}

/**
 * gtd_task_compare:
 * @t1: (nullable): a #GtdTask
 * @t2: (nullable): a #GtdTask
 *
 * Compare @t1 and @t2.
 *
 * Returns: %-1 if @t1 comes before @t2, %1 for the opposite, %0 if they're equal
 */
gint
gtd_task_compare (GtdTask *t1,
                  GtdTask *t2)
{
  GDateTime *dt1;
  GDateTime *dt2;
  gboolean completed1;
  gboolean completed2;
  gchar *txt1;
  gchar *txt2;
  gint p1;
  gint p2;
  gint retval;

  if (!t1 && !t2)
    return  0;
  if (!t1)
    return  1;
  if (!t2)
    return -1;

  /*
   * First, compare by ::complete.
   */
  completed1 = gtd_task_get_complete (t1);
  completed2 = gtd_task_get_complete (t2);
  retval = completed1 - completed2;

  if (retval != 0)
    return retval;

  /*
   * Second, compare by ::priority
   */
  p1 = gtd_task_get_priority (t1);
  p2 = gtd_task_get_priority (t2);

  retval = p2 - p1;

  if (retval != 0)
    return retval;

  /*
   * Third, compare by ::due-date.
   */
  dt1 = gtd_task_get_due_date (t1);
  dt2 = gtd_task_get_due_date (t2);

  if (!dt1 && !dt2)
    retval =  0;
  else if (!dt1)
    retval =  1;
  else if (!dt2)
    retval = -1;
  else
    retval = g_date_time_compare (dt1, dt2);

  if (dt1)
    g_date_time_unref (dt1);
  if (dt2)
    g_date_time_unref (dt2);

  if (retval != 0)
    return retval;

  /*
   * Fourth, compare by ::creation-date.
   */
  dt1 = gtd_task_get_creation_date (t1);
  dt2 = gtd_task_get_creation_date (t2);

  if (!dt1 && !dt2)
    retval =  0;
  else if (!dt1)
    retval =  1;
  else if (!dt2)
    retval = -1;
  else
    retval = g_date_time_compare (dt1, dt2);

  g_clear_pointer (&dt1, g_date_time_unref);
  g_clear_pointer (&dt2, g_date_time_unref);

  if (retval != 0)
    return retval;

  /*
   * If they're equal up to now, compare by title.
   */
  txt1 = txt2 = NULL;

  txt1 = g_utf8_casefold (gtd_task_get_title (t1), -1);
  txt2 = g_utf8_casefold (gtd_task_get_title (t2), -1);

  retval = g_strcmp0 (txt1, txt2);

  g_free (txt1);
  g_free (txt2);

  return retval;
}

/**
 * gtd_task_get_parent:
 * @self: a #GtdTask
 *
 * Retrieves the parent task of @self, or %NULL if none is set.
 *
 * Returns: (transfer none)(nullable): the #GtdTask that is parent of @self
 */
GtdTask*
gtd_task_get_parent (GtdTask *self)
{
  GtdTaskPrivate *priv;

  g_return_val_if_fail (GTD_IS_TASK (self), NULL);

  priv = gtd_task_get_instance_private (self);

  return priv->parent;
}

/**
 * gtd_task_get_substasks:
 * @self: a #GtdTask
 *
 * Retrieves the subtasks of @self, or %NULL if it has no subtasks.
 *
 * Returns: (transfer container)(nullable)(element-type Gtd.Task): the subtasks of @self
 */
GList*
gtd_task_get_substasks (GtdTask *self)
{
  GtdTaskPrivate *priv;

  g_return_val_if_fail (GTD_IS_TASK (self), NULL);

  priv = gtd_task_get_instance_private (self);

  return g_list_copy (priv->subtasks);
}

/**
 * gtd_task_add_subtask:
 * @self: a #GtdTask
 * @subtask: the subtask to be added to @self
 *
 * Adds @subtask as a subtask of @self.
 */
void
gtd_task_add_subtask (GtdTask *self,
                      GtdTask *subtask)
{
  GtdTaskPrivate *priv;

  g_return_if_fail (GTD_IS_TASK (self));
  g_return_if_fail (GTD_IS_TASK (subtask));

  priv = gtd_task_get_instance_private (self);

  if (!g_list_find (priv->subtasks, subtask) && !gtd_task_is_subtask (subtask, self))
    {
      g_signal_emit (self, signals[SUBTASK_ADDED], 0, subtask);
    }
}

/**
 * gtd_task_remove_subtask:
 * @self: a #GtdTask
 * @subtask: the subtask to be removed to @self
 *
 * Removes @subtask from @self.
 */
void
gtd_task_remove_subtask (GtdTask *self,
                         GtdTask *subtask)
{
  GtdTaskPrivate *priv;

  g_return_if_fail (GTD_IS_TASK (self));
  g_return_if_fail (GTD_IS_TASK (subtask));

  priv = gtd_task_get_instance_private (self);

  if (g_list_find (priv->subtasks, subtask))
    {
      g_signal_emit (self, signals[SUBTASK_REMOVED], 0, subtask);
    }
}

/**
 * gtd_task_is_subtask:
 * @self: a #GtdTask
 * @subtask: a #GtdTask
 *
 * Checks if @subtask is a subtask of @self, directly or indirectly.
 *
 * Returns: %TRUE is @subtask is a subtask of @self, %FALSE otherwise
 */
gboolean
gtd_task_is_subtask (GtdTask *self,
                     GtdTask *subtask)
{
  GtdTask *aux;
  GQueue *queue;
  gboolean is_subtask;

  g_return_val_if_fail (GTD_IS_TASK (self), FALSE);
  g_return_val_if_fail (GTD_IS_TASK (subtask), FALSE);

  aux = self;
  queue = g_queue_new ();
  is_subtask = FALSE;

  do
    {
      GtdTaskPrivate *priv;
      GList *l;

      priv = gtd_task_get_instance_private (aux);

      for (l = priv->subtasks; l != NULL; l = l->next)
        {
          /* Found it, no need to continue looping */
          if (l->data == subtask)
            {
              is_subtask = TRUE;
              break;
            }

          g_queue_push_tail (queue, l->data);
        }

      if (is_subtask)
        break;

      aux = g_queue_pop_head (queue);
    }
  while (!g_queue_is_empty (queue));

  g_queue_free (queue);

  return is_subtask;
}
