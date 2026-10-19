/* prelude.c -- library code written in Lisp itself, loaded at
   start-up.  Generated from initprelude in ../lisp.crexx. */
#include "lisp.h"

const char *prelude[] = {
    ";;; Library code written in Lisp itself, loaded at start"
    "-up.\n",
    "\n",
    "(defun endp (x) (null x))\n",
    "(defun complement (f) (lambda (&rest args) (not (apply f"
    " args))))\n",
    "(defun constantly (v) (lambda (&rest args) v))\n",
    "(defun revappend (a b) (append (reverse a) b))\n",
    "(defun nreconc (a b) (append (nreverse a) b))\n",
    "(defun copy-alist (al) (mapcar (lambda (p) (if (consp p)"
    " (cons (car p) (cdr p)) p)) al))\n",
    "(defun member-if (p l) (do ((x l (cdr x))) ((null x) nil"
    ") (when (funcall p (car x)) (return x))))\n",
    "(defun member-if-not (p l) (do ((x l (cdr x))) ((null x)"
    " nil) (unless (funcall p (car x)) (return x))))\n",
    "(defun assoc-if (p al) (dolist (e al nil) (when (and (co"
    "nsp e) (funcall p (car e))) (return e))))\n",
    "(defun find-if-not (p s) (find-if (complement p) s))\n",
    "(defun position-if-not (p s) (position-if (complement p)"
    " s))\n",
    "(defun count-if-not (p s) (count-if (complement p) s))\n",
    "(defun mapcon (f l) (apply #'nconc (maplist f l)))\n",
    "(defun sublis (al tree)\n",
    "  (cond ((atom tree) (let ((p (assoc tree al))) (if p (c"
    "dr p) tree)))\n",
    "        (t (cons (sublis al (car tree)) (sublis al (cdr "
    "tree))))))\n",
    "\n",
    ";;; ---- extended LOOP ---------------------------------"
    "-------------------\n",
    "(defvar *lp-bind* nil)\n",
    "(defvar *lp-head* nil)\n",
    "(defvar *lp-body* nil)\n",
    "(defvar *lp-step* nil)\n",
    "(defvar *lp-init* nil)\n",
    "(defvar *lp-final* nil)\n",
    "(defvar *lp-result* nil)\n",
    "(defvar *lp-end* nil)\n",
    "(defvar *lp-lacc* nil)\n",
    "(defvar *lp-sum* nil)\n",
    "(defvar *lp-cnt* nil)\n",
    "(defvar *lp-max* nil)\n",
    "(defvar *lp-min* nil)\n",
    "\n",
    "(defun %lp-word (x) (if (and x (symbolp x)) (symbol-name"
    " x) \"\"))\n",
    "(defun %lp-is (x &rest names) (member (%lp-word x) names"
    " :test #'string=))\n",
    "(defun %lp-bind (var init) (push (list var init) *lp-bin"
    "d*))\n",
    "(defun %lp-head (form) (push form *lp-head*))\n",
    "(defun %lp-step (form) (push form *lp-step*))\n",
    "\n",
    "(defun %lp-vars (pat)\n",
    "  (cond ((null pat) nil)\n",
    "        ((symbolp pat) (list pat))\n",
    "        (t (append (%lp-vars (car pat)) (%lp-vars (cdr p"
    "at))))))\n",
    "\n",
    "(defun %lp-destr (pat expr)\n",
    "  (cond ((null pat) nil)\n",
    "        ((symbolp pat) (list (list 'setq pat expr)))\n",
    "        (t (append (%lp-destr (car pat) (list 'car expr)"
    ")\n",
    "                   (%lp-destr (cdr pat) (list 'cdr expr)"
    ")))))\n",
    "\n",
    ";; assign the pattern PAT from EXPR each iteration\n",
    "(defun %lp-assign (pat expr)\n",
    "  (if (symbolp pat)\n",
    "      (%lp-head (list 'setq pat expr))\n",
    "      (let ((e (gensym)))\n",
    "        (%lp-bind e nil)\n",
    "        (%lp-head (list 'setq e expr))\n",
    "        (dolist (f (%lp-destr pat e)) (%lp-head f)))))\n",
    "\n",
    "(defun %lp-bindvars (pat)\n",
    "  (dolist (v (%lp-vars pat)) (%lp-bind v nil)))\n",
    "\n",
    "(defun %lp-for (cl)\n",
    "  (let* ((var (cadr cl)) (rest (cddr cl)) (w (car rest))"
    ")\n",
    "    (cond\n",
    "      ((%lp-is w \"IN\")\n",
    "       (let ((l (gensym)) (by nil))\n",
    "         (%lp-bind l (cadr rest))\n",
    "         (%lp-bindvars var)\n",
    "         (setq rest (cddr rest))\n",
    "         (when (%lp-is (car rest) \"BY\") (setq by (cadr"
    " rest)) (setq rest (cddr rest)))\n",
    "         (%lp-head (list 'when (list 'atom l) *lp-end*))\n",
    "         (%lp-assign var (list 'car l))\n",
    "         (%lp-step (list 'setq l (if by (list 'funcall b"
    "y l) (list 'cdr l))))\n",
    "         rest))\n",
    "      ((%lp-is w \"ON\")\n",
    "       (let ((l (gensym)) (by nil))\n",
    "         (%lp-bind l (cadr rest))\n",
    "         (%lp-bindvars var)\n",
    "         (setq rest (cddr rest))\n",
    "         (when (%lp-is (car rest) \"BY\") (setq by (cadr"
    " rest)) (setq rest (cddr rest)))\n",
    "         (%lp-head (list 'when (list 'atom l) *lp-end*))\n",
    "         (%lp-assign var l)\n",
    "         (%lp-step (list 'setq l (if by (list 'funcall b"
    "y l) (list 'cdr l))))\n",
    "         rest))\n",
    "      ((%lp-is w \"ACROSS\")\n",
    "       (let ((v (gensym)) (i (gensym)))\n",
    "         (%lp-bind v (cadr rest))\n",
    "         (%lp-bind i 0)\n",
    "         (%lp-bindvars var)\n",
    "         (%lp-head (list 'when (list '>= i (list 'length"
    " v)) *lp-end*))\n",
    "         (%lp-assign var (list 'aref v i))\n",
    "         (%lp-step (list 'setq i (list '1+ i)))\n",
    "         (cddr rest)))\n",
    "      ((%lp-is w \"=\")\n",
    "       (let ((init (cadr rest)))\n",
    "         (setq rest (cddr rest))\n",
    "         (cond ((%lp-is (car rest) \"THEN\")\n",
    "                (%lp-bind var init)\n",
    "                (%lp-step (list 'setq var (cadr rest)))\n",
    "                (cddr rest))\n",
    "               (t (%lp-bindvars var)\n",
    "                  (%lp-assign var init)\n",
    "                  rest))))\n",
    "      ((%lp-is w \"BEING\")\n",
    "       (let* ((kind (%lp-word (nth 2 rest)))\n",
    "              (tbl (gensym)) (keys (gensym))\n",
    "              (tform (nth 4 rest))\n",
    "              (r (nthcdr 5 rest))\n",
    "              (other nil))\n",
    "         (%lp-bind tbl tform)\n",
    "         (%lp-bind keys (list 'hash-table-keys tbl))\n",
    "         (%lp-bindvars var)\n",
    "         (when (%lp-is (car r) \"USING\")\n",
    "           (setq other (cadr (cadr r)))\n",
    "           (setq r (cddr r))\n",
    "           (%lp-bindvars other))\n",
    "         (%lp-head (list 'when (list 'atom keys) *lp-end"
    "*))\n",
    "         (cond ((string= kind \"HASH-KEYS\")\n",
    "                (%lp-head (list 'setq var (list 'car key"
    "s)))\n",
    "                (when other (%lp-head (list 'setq other "
    "(list 'gethash (list 'car keys) tbl)))))\n",
    "               (t\n",
    "                (%lp-head (list 'setq var (list 'gethash"
    " (list 'car keys) tbl)))\n",
    "                (when other (%lp-head (list 'setq other "
    "(list 'car keys))))))\n",
    "         (%lp-step (list 'setq keys (list 'cdr keys)))\n",
    "         r))\n",
    "      (t\n",
    "       ;; arithmetic: from/to/below/by ...\n",
    "       (let ((start 0) (limit nil) (limkind nil) (step 1"
    ") (down nil))\n",
    "         (do ()\n",
    "             ((not (%lp-is (car rest) \"FROM\" \"DOWNFRO"
    "M\" \"UPFROM\" \"TO\" \"UPTO\" \"BELOW\"\n",
    "                           \"DOWNTO\" \"ABOVE\" \"BY\"))"
    ")\n",
    "           (let ((k (%lp-word (car rest))) (v (cadr rest"
    ")))\n",
    "             (cond ((string= k \"FROM\") (setq start v))\n",
    "                   ((string= k \"UPFROM\") (setq start v"
    "))\n",
    "                   ((string= k \"DOWNFROM\") (setq start"
    " v) (setq down t))\n",
    "                   ((or (string= k \"TO\") (string= k \""
    "UPTO\")) (setq limit v) (setq limkind 'le))\n",
    "                   ((string= k \"BELOW\") (setq limit v)"
    " (setq limkind 'lt))\n",
    "                   ((string= k \"DOWNTO\") (setq limit v"
    ") (setq limkind 'ge) (setq down t))\n",
    "                   ((string= k \"ABOVE\") (setq limit v)"
    " (setq limkind 'gt) (setq down t))\n",
    "                   ((string= k \"BY\") (setq step v)))\n",
    "             (setq rest (cddr rest))))\n",
    "         (%lp-bind var start)\n",
    "         (when limit\n",
    "           (let ((lim (gensym)))\n",
    "             (%lp-bind lim limit)\n",
    "             (%lp-head (list 'when\n",
    "                             (cond ((eq limkind 'le) (li"
    "st '> var lim))\n",
    "                                   ((eq limkind 'lt) (li"
    "st '>= var lim))\n",
    "                                   ((eq limkind 'ge) (li"
    "st '< var lim))\n",
    "                                   (t (list '<= var lim)"
    "))\n",
    "                             *lp-end*))))\n",
    "         (if (numberp step)\n",
    "             (%lp-step (list 'setq var (list (if down '-"
    " '+) var step)))\n",
    "             (let ((stp (gensym)))\n",
    "               (%lp-bind stp step)\n",
    "               (%lp-step (list 'setq var (list (if down "
    "'- '+) var stp)))))\n",
    "         rest)))))\n",
    "\n",
    "(defun %lp-acc-sym (kind)\n",
    "  (cond ((string= kind \"L\") (or *lp-lacc* (progn (setq"
    " *lp-lacc* (gensym)) (%lp-bind *lp-lacc* nil) *lp-lacc*)"
    "))\n",
    "        ((string= kind \"S\") (or *lp-sum* (progn (setq "
    "*lp-sum* (gensym)) (%lp-bind *lp-sum* 0) *lp-sum*)))\n",
    "        ((string= kind \"C\") (or *lp-cnt* (progn (setq "
    "*lp-cnt* (gensym)) (%lp-bind *lp-cnt* 0) *lp-cnt*)))\n",
    "        ((string= kind \"X\") (or *lp-max* (progn (setq "
    "*lp-max* (gensym)) (%lp-bind *lp-max* nil) *lp-max*)))\n",
    "        (t (or *lp-min* (progn (setq *lp-min* (gensym)) "
    "(%lp-bind *lp-min* nil) *lp-min*)))))\n",
    "\n",
    "(defun %lp-setresult (r) (unless *lp-result* (setq *lp-r"
    "esult* r)))\n",
    "\n",
    ";; parse one body clause; returns (forms . rest)\n",
    "(defun %lp-bodyclause (cl)\n",
    "  (let* ((w (%lp-word (car cl))) (form (cadr cl)) (rest "
    "(cddr cl)) (into nil))\n",
    "    (cond\n",
    "      ((member w '(\"COLLECT\" \"COLLECTING\" \"APPEND\""
    " \"APPENDING\" \"NCONC\" \"NCONCING\"\n",
    "                   \"SUM\" \"SUMMING\" \"COUNT\" \"COUNT"
    "ING\" \"MAXIMIZE\" \"MAXIMISE\" \"MAXIMIZING\"\n",
    "                   \"MINIMIZE\" \"MINIMISE\" \"MINIMIZIN"
    "G\")\n",
    "               :test #'string=)\n",
    "       (when (%lp-is (car rest) \"INTO\")\n",
    "         (setq into (cadr rest))\n",
    "         (setq rest (cddr rest)))\n",
    "       (let* ((kind (cond ((member w '(\"COLLECT\" \"COL"
    "LECTING\") :test #'string=) \"L\")\n",
    "                          ((member w '(\"APPEND\" \"APPE"
    "NDING\" \"NCONC\" \"NCONCING\") :test #'string=) \"A\")\n",
    "                          ((member w '(\"SUM\" \"SUMMING"
    "\") :test #'string=) \"S\")\n",
    "                          ((member w '(\"COUNT\" \"COUNT"
    "ING\") :test #'string=) \"C\")\n",
    "                          ((member w '(\"MAXIMIZE\" \"MA"
    "XIMISE\" \"MAXIMIZING\") :test #'string=) \"X\")\n",
    "                          (t \"N\")))\n",
    "              (acc (if into\n",
    "                       (progn (unless (member into (mapc"
    "ar #'car *lp-bind*))\n",
    "                                (%lp-bind into (if (memb"
    "er kind '(\"S\" \"C\") :test #'string=) 0 nil)))\n",
    "                              into)\n",
    "                       (%lp-acc-sym (if (string= kind \""
    "A\") \"L\" kind))))\n",
    "              (forms\n",
    "               (cond\n",
    "                 ((string= kind \"L\")\n",
    "                  (if into (list (list 'setq acc (list '"
    "nconc acc (list 'list form))))\n",
    "                      (list (list 'push form acc))))\n",
    "                 ((string= kind \"A\")\n",
    "                  (if into (list (list 'setq acc (list '"
    "append acc form)))\n",
    "                      (let ((e (gensym)))\n",
    "                        (list (list 'dolist (list e form"
    ") (list 'push e acc))))))\n",
    "                 ((string= kind \"S\") (list (list 'setq"
    " acc (list '+ acc form))))\n",
    "                 ((string= kind \"C\") (list (list 'when"
    " form (list 'setq acc (list '1+ acc)))))\n",
    "                 ((string= kind \"X\")\n",
    "                  (let ((v (gensym)))\n",
    "                    (list (list 'let (list (list v form)"
    ")\n",
    "                                (list 'when (list 'or (l"
    "ist 'null acc) (list '> v acc))\n",
    "                                      (list 'setq acc v)"
    ")))))\n",
    "                 (t\n",
    "                  (let ((v (gensym)))\n",
    "                    (list (list 'let (list (list v form)"
    ")\n",
    "                                (list 'when (list 'or (l"
    "ist 'null acc) (list '< v acc))\n",
    "                                      (list 'setq acc v)"
    "))))))))\n",
    "         (unless into\n",
    "           (%lp-setresult (if (or (string= kind \"L\") ("
    "string= kind \"A\")) (list 'nreverse acc) acc)))\n",
    "         (cons forms rest)))\n",
    "      ((member w '(\"DO\" \"DOING\") :test #'string=)\n",
    "       (let ((forms nil) (r (cdr cl)))\n",
    "         (do () ((not (consp (car r))))\n",
    "           (push (car r) forms)\n",
    "           (setq r (cdr r)))\n",
    "         (cons (reverse forms) r)))\n",
    "      ((string= w \"RETURN\")\n",
    "       (cons (list (list 'return form)) rest))\n",
    "      ((member w '(\"WHEN\" \"IF\" \"UNLESS\") :test #'s"
    "tring=)\n",
    "       (let* ((r (cddr cl)) (then nil) (else nil) (p nil"
    "))\n",
    "         (setq p (%lp-bodyclause r))\n",
    "         (setq then (car p))\n",
    "         (setq r (cdr p))\n",
    "         (do () ((not (%lp-is (car r) \"AND\")))\n",
    "           (setq p (%lp-bodyclause (cdr r)))\n",
    "           (setq then (append then (car p)))\n",
    "           (setq r (cdr p)))\n",
    "         (when (%lp-is (car r) \"ELSE\")\n",
    "           (setq p (%lp-bodyclause (cdr r)))\n",
    "           (setq else (car p))\n",
    "           (setq r (cdr p))\n",
    "           (do () ((not (%lp-is (car r) \"AND\")))\n",
    "             (setq p (%lp-bodyclause (cdr r)))\n",
    "             (setq else (append else (car p)))\n",
    "             (setq r (cdr p))))\n",
    "         (when (%lp-is (car r) \"END\") (setq r (cdr r))"
    ")\n",
    "         (cons (list (if (string= w \"UNLESS\")\n",
    "                         (list 'if form (cons 'progn els"
    "e) (cons 'progn then))\n",
    "                         (list 'if form (cons 'progn the"
    "n) (cons 'progn else))))\n",
    "               r)))\n",
    "      ((string= w \"WHILE\")\n",
    "       (cons (list (list 'unless form *lp-end*)) rest))\n",
    "      ((string= w \"UNTIL\")\n",
    "       (cons (list (list 'when form *lp-end*)) rest))\n",
    "      ((string= w \"ALWAYS\")\n",
    "       (%lp-setresult t)\n",
    "       (cons (list (list 'unless form '(return nil))) re"
    "st))\n",
    "      ((string= w \"NEVER\")\n",
    "       (%lp-setresult t)\n",
    "       (cons (list (list 'when form '(return nil))) rest"
    "))\n",
    "      ((string= w \"THEREIS\")\n",
    "       (let ((v (gensym)))\n",
    "         (cons (list (list 'let (list (list v form)) (li"
    "st 'when v (list 'return v)))) rest)))\n",
    "      (t (error \"LOOP: unknown clause ~S\" (car cl)))))"
    ")\n",
    "\n",
    "(defun %lp-expand (clauses)\n",
    "  (setq *lp-bind* nil *lp-head* nil *lp-body* nil *lp-st"
    "ep* nil *lp-init* nil\n",
    "        *lp-final* nil *lp-result* nil *lp-lacc* nil *lp"
    "-sum* nil *lp-cnt* nil\n",
    "        *lp-max* nil *lp-min* nil)\n",
    "  (setq *lp-end* (list '%lp-end))\n",
    "  (let ((cl clauses))\n",
    "    (do () ((null cl))\n",
    "      (let ((w (%lp-word (car cl))))\n",
    "        (cond\n",
    "          ((string= w \"NAMED\") (setq cl (cddr cl)))\n",
    "          ((or (string= w \"FOR\") (string= w \"AS\")) ("
    "setq cl (%lp-for cl)))\n",
    "          ((string= w \"WITH\")\n",
    "           (let ((var (cadr cl)) (r (cddr cl)) (init nil"
    "))\n",
    "             (when (%lp-is (car r) \"=\") (setq init (ca"
    "dr r)) (setq r (cddr r)))\n",
    "             (%lp-bind var init)\n",
    "             (do () ((not (%lp-is (car r) \"AND\")))\n",
    "               (setq var (cadr r))\n",
    "               (setq r (cddr r))\n",
    "               (setq init nil)\n",
    "               (when (%lp-is (car r) \"=\") (setq init ("
    "cadr r)) (setq r (cddr r)))\n",
    "               (%lp-bind var init))\n",
    "             (setq cl r)))\n",
    "          ((string= w \"REPEAT\")\n",
    "           (let ((n (gensym)))\n",
    "             (%lp-bind n (cadr cl))\n",
    "             (%lp-head (list 'when (list '<= n 0) *lp-en"
    "d*))\n",
    "             (%lp-head (list 'setq n (list '1- n)))\n",
    "             (setq cl (cddr cl))))\n",
    "          ((string= w \"INITIALLY\")\n",
    "           (let ((r (cdr cl)))\n",
    "             (do () ((not (consp (car r))))\n",
    "               (push (car r) *lp-init*)\n",
    "               (setq r (cdr r)))\n",
    "             (setq cl r)))\n",
    "          ((string= w \"FINALLY\")\n",
    "           (let ((r (cdr cl)))\n",
    "             (if (%lp-is (car r) \"RETURN\")\n",
    "                 (progn (push (list 'return (cadr r)) *l"
    "p-final*) (setq r (cddr r)))\n",
    "                 (do () ((not (consp (car r))))\n",
    "                   (push (car r) *lp-final*)\n",
    "                   (setq r (cdr r))))\n",
    "             (setq cl r)))\n",
    "          (t (let ((p (%lp-bodyclause cl)))\n",
    "               (dolist (f (car p)) (push f *lp-body*))\n",
    "               (setq cl (cdr p))))))))\n",
    "  ;; the termination form\n",
    "  (setf (car *lp-end*) 'return)\n",
    "  (setf (cdr *lp-end*) (list (list 'progn (cons 'progn ("
    "reverse *lp-final*)) *lp-result*)))\n",
    "  (list 'let* (reverse *lp-bind*)\n",
    "        (cons 'progn (reverse *lp-init*))\n",
    "        (list 'loop (cons 'progn (append (reverse *lp-he"
    "ad*) (reverse *lp-body*) (reverse *lp-step*))))))\n",
    "\n",
    "(defmacro %loop (&rest clauses) (%lp-expand clauses))\n",
    "\n",
    NULL
};
