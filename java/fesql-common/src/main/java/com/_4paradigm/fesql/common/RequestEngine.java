package com._4paradigm.fesql.common;

import com._4paradigm.fesql.base.BaseStatus;
import com._4paradigm.fesql.type.TypeOuterClass;
import com._4paradigm.fesql.vm.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class RequestEngine implements AutoCloseable {

    private static final Logger logger = LoggerFactory.getLogger(SQLEngine.class);

    private SimpleCatalog catalog;
    private EngineOptions options;
    private Engine engine;
    private RequestRunSession session;
    private CompileInfo compileInfo;
    private PhysicalOpNode plan;


    public RequestEngine(String sql, TypeOuterClass.Database database) throws UnsupportedFesqlException {
        options = new EngineOptions();
        options.set_keep_ir(true);
        options.set_compile_only(true);
        options.set_performance_sensitive(false);
        catalog = new SimpleCatalog();
        session = new RequestRunSession();
        catalog.AddDatabase(database);
        engine = new Engine(catalog, options);

        BaseStatus status = new BaseStatus();
        boolean ok = engine.Get(sql, database.getName(), session, status);
        if (! (ok && status.getMsg().equals("ok"))) {
            throw new UnsupportedFesqlException("SQL parse error: " + status.getMsg());
        }
        status.delete();

        compileInfo = session.GetCompileInfo();
        plan = session.GetPhysicalPlan();
    }

    public PhysicalOpNode getPlan() {
        return plan;
    }

    @Override
    synchronized public void close() throws Exception {
        engine.delete();
        engine = null;

        compileInfo.delete();
        compileInfo = null;

        options.delete();
        options = null;

        plan.delete();
        plan = null;

        session.delete();
        session = null;

        catalog.delete();
        catalog = null;
    }
}